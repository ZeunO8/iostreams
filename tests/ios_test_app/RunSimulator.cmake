message(STATUS "==========================================")
message(STATUS "iOS Simulator Launcher")
message(STATUS "==========================================")
message(STATUS "App: ${APP_PATH}")

find_program(BASH bash REQUIRED)

execute_process(
    COMMAND ${BASH} -c "${XCRUN} simctl list devices | grep '(Booted)' | grep -m1 'iPhone' | grep -o '([-A-F0-9]*)' | tr -d '()'"
    OUTPUT_VARIABLE FOUND_UDID
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT FOUND_UDID)
    message(STATUS "No booted iPhone simulator found. Looking for available devices...")
    execute_process(
        COMMAND ${BASH} -c "${XCRUN} simctl list devices | grep -m1 'iPhone' | grep -o '([-A-F0-9]*)' | tr -d '()'"
        OUTPUT_VARIABLE FOUND_UDID
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT FOUND_UDID)
    message(STATUS "No iPhone simulator devices found. Checking runtimes...")
    execute_process(
        COMMAND ${BASH} -c "${XCRUN} simctl list runtimes | grep -c 'iOS'"
        OUTPUT_VARIABLE RUNTIME_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(RUNTIME_COUNT EQUAL 0 OR RUNTIME_COUNT STREQUAL "")
        message(STATUS "")
        message(FATAL_ERROR
            "No iOS Simulator runtime installed.\n"
            "Install it via Xcode:\n"
            "  1. Open Xcode\n"
            "  2. Go to Xcode > Settings > Platforms\n"
            "  3. Click '+' and install 'iOS Simulator'\n"
            "  4. Re-run: cmake --build tests/ios_test_app/build --target run_simulator"
        )
    endif()

    message(STATUS "Runtimes found but no devices. Creating iPhone 15 device...")
    execute_process(
        COMMAND ${XCRUN} simctl create "iostreams-test" "iPhone 15"
        OUTPUT_VARIABLE CREATE_OUTPUT
        RESULT_VARIABLE CREATE_RESULT
    )

    if(CREATE_RESULT EQUAL 0)
        string(REGEX MATCH "\\([-A-F0-9]+\\)" UDID_MATCH "${CREATE_OUTPUT}")
        string(REPLACE "(" "" FOUND_UDID "${UDID_MATCH}")
        string(REPLACE ")" "" FOUND_UDID "${FOUND_UDID}")
    else()
        message(FATAL_ERROR "Failed to create simulator device: ${CREATE_OUTPUT}")
    endif()
endif()

if(NOT FOUND_UDID)
    message(FATAL_ERROR "Could not find or create an iPhone simulator")
endif()

message(STATUS "Using simulator: ${FOUND_UDID}")

string(FIND "${FOUND_UDID}" "Booted" BOOTED_CHECK)
if(BOOTED_CHECK EQUAL -1)
    message(STATUS "Booting simulator...")
    execute_process(
        COMMAND ${XCRUN} simctl boot "${FOUND_UDID}"
        RESULT_VARIABLE BOOT_RESULT
        TIMEOUT 120
    )
    if(NOT BOOT_RESULT EQUAL 0)
        message(WARNING "Boot command returned non-zero, trying to continue...")
    endif()
    execute_process(COMMAND open -a Simulator)
    execute_process(
        COMMAND ${XCRUN} simctl bootstatus "${FOUND_UDID}" -b
        TIMEOUT 120
    )
endif()

message(STATUS "Installing app...")
execute_process(
    COMMAND ${XCRUN} simctl install "${FOUND_UDID}" "${APP_PATH}"
    RESULT_VARIABLE INSTALL_RESULT
    ERROR_VARIABLE INSTALL_ERROR
)

if(NOT INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install app: ${INSTALL_ERROR}")
endif()

message(STATUS "Installed successfully.")

execute_process(
    COMMAND /usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "${APP_PATH}/Info.plist"
    OUTPUT_VARIABLE BUNDLE_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE PLIST_RESULT
)

if(PLIST_RESULT OR NOT BUNDLE_ID)
    set(BUNDLE_ID "com.test.iostreams")
endif()

message(STATUS "Bundle ID: ${BUNDLE_ID}")
message(STATUS "Launching app in simulator...")
message(STATUS "")
message(STATUS ">>> The app is now running in the iOS Simulator.")
message(STATUS ">>> Verify the HTTP request stats display correctly.")
message(STATUS ">>> Close the Simulator window when done testing.")
message(STATUS "")

execute_process(
    COMMAND ${XCRUN} simctl launch "${FOUND_UDID}" "${BUNDLE_ID}"
    RESULT_VARIABLE LAUNCH_RESULT
    OUTPUT_VARIABLE LAUNCH_OUTPUT
    ERROR_VARIABLE LAUNCH_ERROR
)

if(NOT LAUNCH_RESULT EQUAL 0)
    message(WARNING "Launch returned: ${LAUNCH_ERROR}")
endif()

message(STATUS "")
message(STATUS "App launched. Waiting for you to close the simulator...")
message(STATUS "Press Ctrl+C to exit this script.")

execute_process(
    COMMAND ${XCRUN} simctl spawn "${FOUND_UDID}" launchctl list
    TIMEOUT 999999
)
