# REMOVED: CONFIG =     require 'app/common/config'
Action =     require './action'
# REMOVED: Logger =     require 'app/common/logger'

class EndTurnAction extends Action

  @type:"EndTurnAction"

  constructor: () ->
    @type ?= EndTurnAction.type
    super

  isRemovableDuringScrubbing: () ->
    return false

  _execute: () ->
    @getGameSession().p_endTurn()

module.exports = EndTurnAction
