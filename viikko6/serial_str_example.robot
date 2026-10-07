*** Settings ***
Library    SerialLibrary
Suite Setup    Connect Serial
Suite Teardown    Disconnect Serial

*** Variables ***
${COM}       COM7
${BAUD}      115200
${BOARD}     nRF
${VALID_TIME}    000120X
${VALID_RESULT}  80X
${INVALID_TIME}  001067X
${INVALID_RESULT}  -3X

*** Test Cases ***
Valid Time String Returns Seconds
    Prepare Serial Test
    Write Data    ${VALID_TIME}    encoding=ascii
    ${read}=    Read Until    terminator=58    encoding=ascii
    Log To Console    Received ${read}
    Should Be Equal As Strings    ${read}    ${VALID_RESULT}

Invalid Seconds Return Error Code
    Prepare Serial Test
    Write Data    ${INVALID_TIME}    encoding=ascii
    ${read}=    Read Until    terminator=58    encoding=ascii
    Log To Console    Received ${read}
    Should Be Equal As Strings    ${read}    ${INVALID_RESULT}

Valid And Invalid Seconds
    Prepare Serial Test
    Write Data    000001X    encoding=ascii
    ${read}=    Read Until    terminator=58    encoding=ascii
    Log To Console    Received ${read}
    Should Be Equal As Strings    ${read}    1X
    Prepare Serial Test
    Write Data    000060X    encoding=ascii
    ${read}=    Read Until    terminator=58    encoding=ascii
    Log To Console    Received ${read}
    Should Be Equal As Strings    ${read}    -3X

Correct Time 141205
    Send And Expect     141205X     51125X

Parse Minutes And Seconds
    Send And Expect     000120X     80X

Accepts Maximum Valid Time
    Send And Expect     235959X     86399X

Rejects Hour Above Maximum
    Send And Expect     240000X     -3X

Rejects Minute Above Maximum
    Send And Expect     006000X     -3X

Rejects Second Above Maximum
    Send And Expect     000060X     -3X

Rejects Zero Time
    Send And Expect     000000X     -4X

Rejects Too Short Time
    Send And Expect     12345X      -1X

Rejects Too Long Time
    Send And Expect     1234567X    -1X

Rejects Non Numeric Characters
    Send And Expect     12A405X     -2X

*** Keywords ***
Connect Serial
    Log To Console    Connecting to ${BOARD} on ${COM}
    Add Port    ${COM}    baudrate=${BAUD}    encoding=ascii
    Port Should Be Open    ${COM}
    Reset Input Buffer
    Reset Output Buffer

Prepare Serial Test
    Sleep    700ms
    Reset Input Buffer

Send And Expect
    [Arguments]    ${request}    ${expected}
    Prepare Serial Test
    Write Data    ${request}    encoding=ascii
    ${read}=    Read Until    terminator=58    encoding=ascii
    Log To Console    Received ${read}
    Should Be Equal As Strings    ${read}    ${expected}

Disconnect Serial
    Delete Port    ${COM}
