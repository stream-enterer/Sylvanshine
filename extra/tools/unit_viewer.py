#!/usr/bin/env python3
"""Duelyst Unit Sprite Viewer - uses manifest.tsv as source of truth"""

import http.server
import json
import os
import sys
import threading

PORT = 8080
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.expanduser("~/.local/git/Sylvanshine/data")
MANIFEST_PATH = os.path.join(DATA_DIR, "units", "manifest.tsv")
NOTES_PATH = os.path.join(SCRIPT_DIR, "card_notes.json")


def load_manifest(path):
    units = []
    with open(path) as f:
        for line in f:
            if line.startswith("unit\t"):
                continue
            parts = line.strip().split("\t")
            if len(parts) >= 3:
                folder = parts[0]
                anims = [a.strip() for a in parts[2].split(",") if a.strip()]
                units.append({"folder": folder, "animations": anims, "name": folder})
    return sorted(units, key=lambda u: u["folder"])


def load_notes():
    if os.path.exists(NOTES_PATH):
        try:
            with open(NOTES_PATH) as f:
                return json.load(f)
        except:
            pass
    return {}


def save_notes(notes):
    with open(NOTES_PATH, "w") as f:
        json.dump(notes, f, indent=2)


HTML = """<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>Unit Viewer</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font:13px/1.4 system-ui,sans-serif;background:#1e1e1e;color:#ccc;display:flex;height:100vh}
#sidebarcont{width:220px;display:flex;flex-direction:column;background:#252525;border-right:1px solid #383838}
#search{background:#2a2a2a;border:1px solid #3a3a3a;color:#ccc;padding:6px 10px;font-size:12px;width:100%;border-radius:0}
#search:focus{outline:none;border-color:#5a5a5a}
#search::placeholder{color:#666}
#sidebar{flex:1;overflow-y:auto;overflow-x:hidden;
  scrollbar-width:thin;scrollbar-color:#555 #2a2a2a}
#sidebar::-webkit-scrollbar{width:8px}
#sidebar::-webkit-scrollbar-track{background:#2a2a2a}
#sidebar::-webkit-scrollbar-thumb{background:#555;border-radius:4px}
#sidebar::-webkit-scrollbar-thumb:hover{background:#666}
#sidebar div{padding:6px 10px;cursor:pointer;border-bottom:1px solid #2a2a2a;white-space:nowrap;text-overflow:ellipsis;overflow:hidden}
#sidebar div:nth-child(odd){background:#2d2d2d}
#sidebar div:nth-child(even){background:#262626}
#sidebar div:hover{background:#3a3a3a}
#sidebar div.sel{background:#4a4a4a;color:#fff}
#sidebar div.hidden{display:none}
#main{flex:1;display:flex;flex-direction:column;padding:12px}
#topbar{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px}
#info{color:#999;font-size:12px}
#scalesel{background:#3a3a3a;border:1px solid #4a4a4a;color:#bbb;padding:4px 8px;border-radius:3px;font-size:12px}
#stage{flex:1;background:#2d2d2d;display:flex;align-items:center;justify-content:center;border-radius:4px;min-height:200px}
canvas{background:#2d2d2d;image-rendering:pixelated;image-rendering:crisp-edges}
#controls{margin-top:10px;display:flex;gap:6px;flex-wrap:wrap}
#controls button{padding:4px 10px;background:#3a3a3a;border:1px solid #4a4a4a;color:#bbb;cursor:pointer;border-radius:3px;font-size:12px}
#controls button:hover{background:#4a4a4a;color:#fff}
#controls button.active{background:#5a5a5a;color:#fff;border-color:#6a6a6a}
#notespanel{margin-top:10px;height:25vh;display:flex;flex-direction:column}
#notespanel label{color:#888;font-size:11px;margin-bottom:4px}
#notes{flex:1;background:#2a2a2a;border:1px solid #3a3a3a;color:#ccc;padding:8px;border-radius:4px;resize:none;font:12px/1.4 monospace}
#notes:focus{outline:none;border-color:#5a5a5a}
</style></head><body>
<div id="sidebarcont">
<input type="text" id="search" placeholder="Search units...">
<div id="sidebar"></div>
</div>
<div id="main">
<div id="topbar">
<div id="info">Loading...</div>
<select id="scalesel"><option value="1">1x</option><option value="2">2x</option><option value="3" selected>3x</option></select>
</div>
<div id="stage"><canvas id="c" width="200" height="200"></canvas></div>
<div id="controls"></div>
<div id="notespanel">
<label>Notes for <span id="noteunit">-</span></label>
<textarea id="notes" placeholder="Enter notes for this unit..."></textarea>
</div>
</div>
<script>
let units=[],cur=null,anims={},curAnim=null,frame=0,lastT=0,ctx=document.getElementById('c').getContext('2d'),img=null;
let scale=3,notes={},saveTimer=null,curIdx=0,filtered=[];

async function init(){
  try{
    const [unitsResp,notesResp]=await Promise.all([fetch('/api/units'),fetch('/api/notes')]);
    if(!unitsResp.ok)throw new Error('Failed to load units');
    units=await unitsResp.json();
    if(notesResp.ok)notes=await notesResp.json();
  }catch(e){
    document.getElementById('info').textContent='Error: '+e.message;
    return;
  }
  const sb=document.getElementById('sidebar');
  units.forEach((u,i)=>{
    const d=document.createElement('div');
    d.textContent=u.name;
    d.dataset.idx=i;
    d.onclick=()=>select(i);
    sb.appendChild(d);
  });
  filtered=[...Array(units.length).keys()];
  document.getElementById('scalesel').onchange=e=>{scale=+e.target.value;updateCanvas();};
  document.getElementById('notes').oninput=debounceNotes;
  document.getElementById('search').oninput=e=>filterUnits(e.target.value);
  document.addEventListener('keydown',handleKey);
  if(units.length)select(0);
}

function filterUnits(q){
  const query=q.toLowerCase();
  const sb=document.getElementById('sidebar');
  filtered=[];
  units.forEach((u,i)=>{
    const el=sb.children[i];
    const match=!query||u.folder.toLowerCase().includes(query);
    el.classList.toggle('hidden',!match);
    if(match)filtered.push(i);
  });
  if(filtered.length&&!filtered.includes(curIdx)){
    select(filtered[0]);
  }
}

function handleKey(e){
  const notes=document.getElementById('notes');
  const search=document.getElementById('search');
  const inNotes=document.activeElement===notes;
  const inSearch=document.activeElement===search;
  
  if(e.key==='ArrowUp'||e.key==='ArrowDown'){
    e.preventDefault();
    if(!filtered.length)return;
    const pos=filtered.indexOf(curIdx);
    let newPos=pos;
    if(e.key==='ArrowUp')newPos=Math.max(0,pos-1);
    else newPos=Math.min(filtered.length-1,pos+1);
    if(newPos!==pos)select(filtered[newPos]);
  }else if(!inNotes&&!inSearch&&e.key.length===1&&!e.ctrlKey&&!e.metaKey){
    search.focus();
    search.value+=e.key;
    filterUnits(search.value);
    e.preventDefault();
  }else if(!inNotes&&e.key==='Backspace'){
    search.focus();
    search.value=search.value.slice(0,-1);
    filterUnits(search.value);
    e.preventDefault();
  }else if(e.key==='Escape'){
    search.value='';
    filterUnits('');
    search.blur();
    notes.blur();
  }
}

function debounceNotes(){
  if(saveTimer)clearTimeout(saveTimer);
  saveTimer=setTimeout(saveNotes,500);
}

async function saveNotes(){
  if(!cur)return;
  notes[cur.folder]=document.getElementById('notes').value;
  try{await fetch('/api/notes',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(notes)});}catch(e){}
}

function loadNoteForUnit(){
  document.getElementById('noteunit').textContent=cur?cur.name:'-';
  document.getElementById('notes').value=cur&&notes[cur.folder]?notes[cur.folder]:'';
}

async function select(i){
  curIdx=i;
  cur=units[i];
  document.querySelectorAll('#sidebar div').forEach(d=>d.classList.toggle('sel',+d.dataset.idx===i));
  const selEl=document.querySelector('#sidebar div.sel');
  if(selEl)selEl.scrollIntoView({block:'nearest'});
  document.getElementById('info').textContent=cur.folder+' — '+cur.animations.length+' animations';
  loadNoteForUnit();
  
  img=new Image();
  img.src='/units/'+cur.folder+'/spritesheet.png';
  try{
    await new Promise((res,rej)=>{img.onload=res;img.onerror=()=>rej(new Error('sprite load failed'));});
  }catch(e){
    document.getElementById('info').textContent=cur.folder+' — ERROR loading sprite';
    return;
  }
  
  try{
    const resp=await fetch('/units/'+cur.folder+'/animations.txt');
    if(!resp.ok)throw new Error('anim fetch failed');
    const txt=await resp.text();
    anims={};
    txt.trim().split('\\n').forEach(line=>{
      const p=line.split('\\t');
      if(p.length<3||!p[2])return;
      const vals=p[2].split(',').map(Number);
      const frames=[];
      for(let j=0;j<vals.length;j+=5){
        frames.push({x:vals[j+1],y:vals[j+2],w:vals[j+3],h:vals[j+4]});
      }
      if(frames.length)anims[p[0]]={fps:+p[1]||12,frames};
    });
  }catch(e){
    document.getElementById('info').textContent=cur.folder+' — ERROR loading animations';
    return;
  }
  
  const ctrl=document.getElementById('controls');
  ctrl.innerHTML='';
  cur.animations.forEach(a=>{
    const b=document.createElement('button');
    b.textContent=a;
    b.onclick=()=>playAnim(a);
    if(!anims[a])b.style.opacity='0.4';
    ctrl.appendChild(b);
  });
  
  const first=cur.animations.find(a=>a==='breathing'&&anims[a])||cur.animations.find(a=>anims[a])||Object.keys(anims)[0];
  if(first)playAnim(first);
}

function updateCanvas(){
  if(!curAnim||!curAnim.frames.length)return;
  const f=curAnim.frames[0];
  const c=document.getElementById('c');
  c.width=f.w*scale;c.height=f.h*scale;
  ctx.imageSmoothingEnabled=false;
}

function playAnim(name){
  if(!anims[name])return;
  curAnim=anims[name];frame=0;lastT=0;
  document.querySelectorAll('#controls button').forEach(b=>b.classList.toggle('active',b.textContent===name));
  updateCanvas();
}

function render(t){
  requestAnimationFrame(render);
  if(!curAnim||!curAnim.frames.length||!img||!img.complete)return;
  const dt=t-lastT;
  if(dt>1000/curAnim.fps){
    lastT=t;
    frame=(frame+1)%curAnim.frames.length;
  }
  const f=curAnim.frames[frame];
  const c=ctx.canvas;
  ctx.fillStyle='#2d2d2d';
  ctx.fillRect(0,0,c.width,c.height);
  ctx.drawImage(img,f.x,f.y,f.w,f.h,0,0,f.w*scale,f.h*scale);
}

init();
requestAnimationFrame(render);
</script></body></html>"""


class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(HTML.encode())
        elif self.path == "/api/units":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(self.server.units).encode())
        elif self.path == "/api/notes":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(load_notes()).encode())
        elif self.path.startswith("/units/"):
            fpath = os.path.join(DATA_DIR, "units", self.path[7:])
            if os.path.isfile(fpath):
                self.send_response(200)
                self.send_header(
                    "Content-Type",
                    "image/png" if fpath.endswith(".png") else "text/plain",
                )
                self.end_headers()
                with open(fpath, "rb") as f:
                    self.wfile.write(f.read())
            else:
                self.send_error(404)
        else:
            self.send_error(404)

    def do_POST(self):
        if self.path == "/api/notes":
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length)
            try:
                data = json.loads(body)
                save_notes(data)
                self.send_response(200)
                self.end_headers()
            except Exception as e:
                self.send_error(400, str(e))
        else:
            self.send_error(404)


def main():
    print(f"Data: {DATA_DIR}")
    print(f"Notes: {NOTES_PATH}")

    if not os.path.isdir(DATA_DIR):
        print(f"ERROR: Data directory not found: {DATA_DIR}")
        sys.exit(1)

    if not os.path.exists(MANIFEST_PATH):
        print(f"ERROR: Manifest not found: {MANIFEST_PATH}")
        sys.exit(1)

    print(f"Loading manifest...")
    units = load_manifest(MANIFEST_PATH)
    print(f"  {len(units)} units")

    server = http.server.HTTPServer(("", PORT), Handler)
    server.units = units

    print(f"\nReady: http://localhost:{PORT}")
    print("Ctrl+C to stop\n")

    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()
    try:
        while True:
            t.join(1)
    except KeyboardInterrupt:
        print("\nStopping...")
        server.shutdown()


if __name__ == "__main__":
    main()
