*** Settings ***
Library    SerialLibrary

*** Variables ***
${COM}      COM7
${BAUD}     115200
${BOARD}    nRF

*** Test Cases ***
Connect Serial
    Log To Console    Connecting to ${BOARD} on ${COM}
    Add Port    ${COM}    baudrate=${BAUD}    encoding=ascii
    Port Should Be Open    ${COM}
    Reset Input Buffer
    Reset Output Buffer

Disconnect Serial
    [Teardown]    Delete Port    ${COM}
