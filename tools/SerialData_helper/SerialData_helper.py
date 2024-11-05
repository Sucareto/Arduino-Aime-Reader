# 对读卡器抓包数据进行标注的工具
# 适用于 Eltima Software/Serial Port Monitor 抓包导出的 csv 文件

# 自设标记：
# >>> 主机向设备发送的数据
# <<< 设备向主机回复的数据
# []  数据块
# {}  转义符和需要转义的字节
# ()  length，或非 0 的 status


from csv import DictReader
from sys import argv

cmd = { # 命令标记的定义
    "30": "CMD_GET_FW_VERSION",
    "32": "CMD_GET_HW_VERSION",
    "40": "CMD_START_POLLING",
    "41": "CMD_STOP_POLLING",
    "42": "CMD_CARD_DETECT",
    "43": "CMD_CARD_SELECT",
    "44": "CMD_CARD_HALT",
    "50": "CMD_MIFARE_KEY_SET_A",
    "51": "CMD_MIFARE_AUTHORIZE_A",
    "52": "CMD_MIFARE_READ",
    "53": "CMD_MIFARE_WRITE",
    "54": "CMD_MIFARE_KEY_SET_B",
    "55": "CMD_MIFARE_AUTHORIZE_B",
    "60": "CMD_TO_UPDATER_MODE",
    "61": "CMD_SEND_HEX_DATA",
    "62": "CMD_TO_NORMAL_MODE",
    "63": "CMD_SEND_BINDATA_INIT",
    "64": "CMD_SEND_BINDATA_EXEC",
    "70": "CMD_FELICA_PUSH",
    "71": "CMD_FELICA_THROUGH",
    "80": "CMD_EXT_BOARD_LED",
    "81": "CMD_EXT_BOARD_LED_RGB",
    "82": "CMD_EXT_BOARD_LED_RGB_UNKNOWN",
    "F0": "CMD_EXT_BOARD_INFO",
    "F2": "CMD_EXT_FIRM_SUM",
    "F3": "CMD_EXT_SEND_HEX_DATA",
    "F4": "CMD_EXT_TO_BOOT_MODE",
    "F5": "CMD_EXT_TO_NORMAL_MODE",
}


def format_hex_data(data, function):

    parts = data.upper().split()  # 转换大写，切分数据

    # 处理转义符 D0
    formatted_parts = []
    i = 0
    while i < len(parts):
        if parts[i] == 'D0' and i + 1 < len(parts):
            # 找到 D0，将其和后一位 hex 用 {} 包围
            formatted_parts.append(f"{{D0 {parts[i + 1]}}}")
            i += 2  # 跳过下一位
        else:
            formatted_parts.append(parts[i])
            i += 1

    # 添加 cmd 注释
    comments = cmd[formatted_parts[4]]
    # 检查 cmd（第5位）并用 [] 包围
    formatted_parts[4] = f"[{formatted_parts[4]}]"

    # 检查 status（第6位）是否非零，在 IRP_MJ_READ 时用 () 包围
    if function == 'IRP_MJ_READ' and int(formatted_parts[5], 16) != 0:
        formatted_parts[5] = f"({formatted_parts[5]})"

    if function == 'IRP_MJ_WRITE':
        prefix = f"\n// {comments}\n>>>"
    elif function == 'IRP_MJ_READ':
        prefix = '<<<'

    return f"{prefix} {' '.join(formatted_parts)}"


def main():
    if len(argv) < 2:
        print("请提供输入文件名")
        exit(1)

    input_file = argv[1]
    output_file = f"{input_file}_done.txt"  # 输出文件名

    with open(input_file, mode='r', newline='', encoding='utf8', errors='ignore') as csvfile, open(output_file, 'w') as outfile:
        reader = DictReader(csvfile, delimiter=';')  # 使用分号作为分隔符

        # 按顺序处理每行数据
        for row in reader:
            if row['Status'] == 'STATUS_SUCCESS':
                formatted_data = format_hex_data(row['Data'], row['Function'])
                # 输出到文件
                outfile.write(formatted_data + "\n")


if __name__ == "__main__":
    main()
