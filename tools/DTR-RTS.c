#include <windows.h>

int main() {
    HANDLE com;
    com = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL, 0);
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(com, &dcbSerialParams);
    dcbSerialParams.BaudRate = 115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(com, &dcbSerialParams);
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutConstant = 1;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 1;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    SetCommTimeouts(com, &timeouts);
    EscapeCommFunction(com, SETDTR);
    EscapeCommFunction(com, SETRTS);

    com = CreateFile("\\\\.\\COM12", GENERIC_READ | GENERIC_WRITE, 0, 0,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(com, &dcbSerialParams);
    dcbSerialParams.BaudRate = 38400;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(com, &dcbSerialParams);
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutConstant = 1;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 1;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    SetCommTimeouts(com, &timeouts);
    EscapeCommFunction(com, SETDTR);
    EscapeCommFunction(com, SETRTS);

    return 0;
}
