#include "w25q64.h"

// 读取芯片ID的函数
// 返回值应该是 0xEF4017 (Winbond W25Q64)
uint32_t W25Q_ReadID(void) {
    uint8_t cmd = 0x9F; // JEDEC ID 指令
    uint8_t rx_data[3] = {0, 0, 0};
    uint32_t id = 0;
    // 1. 拉低片选，开始通信
    W25Q_CS_LOW();
    // 2. 发送指令 0x9F
    // 注意：使用的是 &hspi2
    if (HAL_SPI_Transmit(&hspi2, &cmd, 1, 100) != HAL_OK) {
        W25Q_CS_HIGH();
        return 0; // 发送失败
    }
    // 3. 接收 3 个字节 (厂商ID, 存储类型, 容量)
    if (HAL_SPI_Receive(&hspi2, rx_data, 3, 100) != HAL_OK) {
        W25Q_CS_HIGH();
        return 0; // 接收失败
    }
    // 4. 拉高片选，结束通信
    W25Q_CS_HIGH();
    // 5. 组合结果
    // rx_data[0] = 0xEF (Winbond)
    // rx_data[1] = 0x40 (SPI)
    // rx_data[2] = 0x17 (64M-bit = 8MB)
    id = (rx_data[0] << 16) | (rx_data[1] << 8) | rx_data[2];
    return id;
}

// 写使能 (写入/擦除前必须调用)
void W25Q_WriteEnable(void) {
    W25Q_CS_LOW();
    uint8_t cmd = 0x06;
    HAL_SPI_Transmit(&hspi2, &cmd, 1, 100);
    W25Q_CS_HIGH();
}

// 等待忙状态结束 (写入后必须等待)
void W25Q_WaitForWriteEnd(void) {
    uint8_t status = 0;
    uint8_t cmd = 0x05; // 读状态寄存器1
    do {
        W25Q_CS_LOW();
        HAL_SPI_Transmit(&hspi2, &cmd, 1, 100);
        HAL_SPI_Receive(&hspi2, &status, 1, 100);
        W25Q_CS_HIGH();
        // 状态寄存器最低位(BUSY)为1表示忙
    } while ((status & 0x01) == 0x01);
}

// 擦除一个扇区 (4KB)
// addr: 扇区地址 (0, 4096, 8192...)
void W25Q_EraseSector(uint32_t addr) {
    W25Q_WriteEnable();
    W25Q_WaitForWriteEnd();

    W25Q_CS_LOW();
    uint8_t cmd[4];
    cmd[0] = 0x20; // 扇区擦除指令
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    HAL_SPI_Transmit(&hspi2, cmd, 4, 100);
    W25Q_CS_HIGH();

    W25Q_WaitForWriteEnd(); // 擦除需要时间，必须死等
}

// 页写入 (最多一次写 256 字节)
void W25Q_PageProgram(uint32_t addr, const uint8_t *data, uint16_t size) {
    W25Q_WriteEnable();
    
    W25Q_CS_LOW();
    uint8_t cmd[4];
    cmd[0] = 0x02; // 页编程指令
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    HAL_SPI_Transmit(&hspi2, cmd, 4, 100);
    HAL_SPI_Transmit(&hspi2, (uint8_t*)data, size, 1000);
    W25Q_CS_HIGH();

    W25Q_WaitForWriteEnd();
}

// 【封装】写入大量数据 (自动处理翻页和擦除)
// start_addr: W25Q64里的起始地址 (比如 0)
// data: 图片数组指针
// size: 数组总大小
void W25Q_WriteData_Smart(uint32_t start_addr, const uint8_t *data, uint32_t size)
{
    uint32_t addr = start_addr;
    uint32_t remain = size;
    uint32_t index = 0;

    /* ---------- 1. 擦除涉及到的扇区（4KB对齐） ---------- */
    uint32_t sector_start = start_addr & ~0xFFF;               // 起始扇区地址
    uint32_t end_addr     = start_addr + size - 1;             // 最后字节地址
    uint32_t sector_end   = end_addr & ~0xFFF;                 // 结束扇区地址
    uint32_t sector_count = (sector_end - sector_start) / 4096 + 1;

    for (uint32_t i = 0; i < sector_count; i++) {
        uint32_t sector_addr = sector_start + i * 4096;
        W25Q_EraseSector(sector_addr);
        // 可选：LED 闪烁指示正在擦除
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }

    /* ---------- 2. 按页写入（处理页内偏移） ---------- */
    while (remain > 0) {
        // 计算当前页剩余空间
        uint32_t page_offset = addr & 0xFF;               // 页内偏移 (0~255)
        uint32_t page_remain = 256 - page_offset;         // 当前页剩余可写字节数

        uint32_t write_len = (remain < page_remain) ? remain : page_remain;

        W25Q_PageProgram(addr, &data[index], (uint16_t)write_len);

        addr   += write_len;
        index  += write_len;
        remain -= write_len;
    }
}
// 读函数
void W25Q_ReadData(uint32_t addr, uint8_t *data, uint32_t size) {
    W25Q_CS_LOW();
    uint8_t cmd[4];
    cmd[0] = 0x03; // 读取指令
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    HAL_SPI_Transmit(&hspi2, cmd, 4, 100);
    HAL_SPI_Receive(&hspi2, data, size, 1000);
    W25Q_CS_HIGH();
}
