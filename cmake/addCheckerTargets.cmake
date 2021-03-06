#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# Only enable if built as top-level project
if (ramses-logic_ENABLE_CODE_STYLE AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    if (rlogic_PYTHON3)
        file(GLOB_RECURSE RL_CHECKER_SCRIPTS ${PROJECT_SOURCE_DIR}/ci/scripts/code_style_checker/*.py)

        add_custom_target(RL_CHECK_CODE_STYLE
            COMMAND ${rlogic_PYTHON3} ${PROJECT_SOURCE_DIR}/ci/scripts/code_style_checker/check_all_styles.py
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            SOURCES ${RL_CHECKER_SCRIPTS}
            )
        set_property(TARGET RL_CHECK_CODE_STYLE PROPERTY FOLDER "CMakePredefinedTargets")

        message(STATUS " + RL_CHECK_CODE_STYLE")
    else()
        message(STATUS " - RL_CHECK_CODE_STYLE [missing python3]")
    endif()
endif()
