/* This is some sort of LCD display found on various cabinets. It is driven
   directly by amdaemon, and it has something to do with displaying the status
   of electronic payments.

   Part number in schematics is "VFD GP1232A02A FUTABA".

   Little else about this board is known. Black-holing the RS232 comms that it
   receives seems to be sufficient for the time being. */

#include <windows.h>

#include <assert.h>
#include <stdint.h>

#include "board/vfd.h"

#include "hook/iohook.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT vfd_handle_irp(struct irp *irp);

static struct uart vfd_uart;
static uint8_t vfd_written[512];
static uint8_t vfd_readable[512];
static uint8_t vfd_enable, vfd_redirect;
static HANDLE com_device;
UINT codepage;

HRESULT vfd_hook_init(unsigned int port_no)
{
    vfd_enable = GetPrivateProfileIntW(L"vfd", L"enable", 1, L".\\segatools.ini");
    vfd_redirect = GetPrivateProfileIntW(L"vfd", L"redirect", 0, L".\\segatools.ini");
    if (!vfd_enable)
    {
        return S_FALSE;
    }

    char com_name[10];
    // sprintf(com_name, "\\\\.\\COM%d", port_no);
    // com_device = CreateFile(com_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    // if (com_device != INVALID_HANDLE_VALUE)
    // {
    //     CloseHandle(com_device);
    //     if (!vfd_redirect)
    //     {
    //         return S_FALSE;
    //     }
    // }

    uart_init(&vfd_uart, port_no);
    vfd_uart.written.bytes = vfd_written;
    vfd_uart.written.nbytes = sizeof(vfd_written);
    vfd_uart.readable.bytes = vfd_readable;
    vfd_uart.readable.nbytes = sizeof(vfd_readable);

    if (vfd_redirect) // 将 vfd 串口数据重写到另一个 com，用于调试
    {
        dprintf("VFD: redirect enable, target COM%d.\n", vfd_redirect);
        sprintf(com_name, "\\\\.\\COM%d", vfd_redirect);
        com_device = CreateFile(com_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (com_device != INVALID_HANDLE_VALUE)
        {
            dprintf("VFD: CreateFile OK.\n");
            DCB dcbSerialParams = {0};
            dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
            GetCommState(com_device, &dcbSerialParams);
            dcbSerialParams.BaudRate = 115200;
            dcbSerialParams.ByteSize = 8;
            dcbSerialParams.StopBits = ONESTOPBIT;
            dcbSerialParams.Parity = NOPARITY;
            SetCommState(com_device, &dcbSerialParams);

            // COMMTIMEOUTS timeouts = {0};
            // timeouts.ReadIntervalTimeout = 1;
            // timeouts.ReadTotalTimeoutConstant = 1;
            // timeouts.ReadTotalTimeoutMultiplier = 1;
            // timeouts.WriteTotalTimeoutConstant = 1;
            // timeouts.WriteTotalTimeoutMultiplier = 1;
            // SetCommTimeouts(com_device, &timeouts);

            EscapeCommFunction(com_device, SETDTR);
            EscapeCommFunction(com_device, SETRTS);
        }
        else
        {
            dprintf("VFD: CreateFile error.\n");
        }
    }
    codepage = GetACP(); // 获取当前系统的 Code page，用于后续转码显示
    dprintf("VFD: hook enabled.\n");
    return iohook_push_handler(vfd_handle_irp);
}

static HRESULT vfd_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&vfd_uart, irp))
    {
        return iohook_invoke_next(irp);
    }

    hr = uart_handle_irp(&vfd_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE)
    {
        return hr;
    }

    if (vfd_redirect && com_device != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if (WriteFile(com_device, vfd_uart.written.bytes, vfd_uart.written.pos, &bytesWritten, NULL))
        {
            dprintf("VFD: WriteFile %lu.\n", bytesWritten);
        }
    }
    else
    {
        uint8_t cmd = 0;
        uint8_t str_1[512]; // 两行数据的缓冲区
        uint8_t str_2[512];
        uint8_t str_1_len = 0;
        uint8_t str_2_len = 0;
        for (size_t i = 0; i < vfd_uart.written.pos; i++)
        {
            if (vfd_uart.written.bytes[i] == 0x1B)
            {
                i++;
                cmd = vfd_uart.written.bytes[i];
                if (cmd == 0x30)
                {
                    i += 3;
                }
                else if (cmd == 0x50)
                {
                    i++;
                }
                continue;
            }
            if (cmd == 0x30)
            {
                str_1[str_1_len++] = vfd_uart.written.bytes[i];
            }
            else if (cmd == 0x50)
            {
                str_2[str_2_len++] = vfd_uart.written.bytes[i];
            }
        }

        if (str_1_len)
        {
            str_1[str_1_len++] = '\0';
            if (codepage != 932)
            { // 如果非日文系统，则转码后输出 https://en.wikipedia.org/wiki/Code_page_932_(Microsoft_Windows)
                WCHAR buffer[512];
                MultiByteToWideChar(932, 0, (LPCSTR)str_1, str_1_len, buffer, str_1_len);
                char str_recode[str_1_len * 3];
                WideCharToMultiByte(codepage, 0, buffer, str_1_len, str_recode, str_1_len * 3, NULL, NULL);
                dprintf("VFD: %s\n", str_recode);
            }
            else
            {
                dprintf("VFD: %s\n", str_1);
            }
        }

        if (str_2_len)
        {
            str_2[str_2_len++] = '\0';
            if (codepage != 932)
            {
                WCHAR buffer[512];
                MultiByteToWideChar(932, 0, (LPCSTR)str_2, str_2_len, buffer, str_2_len);
                char str_recode[str_2_len * 3];
                WideCharToMultiByte(codepage, 0, buffer, str_2_len, str_recode, str_2_len * 3, NULL, NULL);
                dprintf("VFD: %s\n", str_recode);
            }
            else
            {
                dprintf("VFD: %s\n", str_2);
            }
        }
    }

    vfd_uart.written.pos = 0;

    return hr;
}
