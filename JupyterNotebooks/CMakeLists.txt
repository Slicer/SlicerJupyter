
#-----------------------------------------------------------------------------
set(MODULE_NAME JupyterNotebooks)

#-----------------------------------------------------------------------------
set(MODULE_PYTHON_SCRIPTS
  ${MODULE_NAME}.py
  ${MODULE_NAME}Lib/__init__
  ${MODULE_NAME}Lib/interactive_view_widget
  ${MODULE_NAME}Lib/cli
  ${MODULE_NAME}Lib/files
  ${MODULE_NAME}Lib/display
  ${MODULE_NAME}Lib/widgets
  )

set(MODULE_PYTHON_RESOURCES
  )

#-----------------------------------------------------------------------------
slicerMacroBuildScriptedModule(
  NAME ${MODULE_NAME}
  SCRIPTS ${MODULE_PYTHON_SCRIPTS}
  RESOURCES ${MODULE_PYTHON_RESOURCES}
  WITH_GENERIC_TESTS
  )
