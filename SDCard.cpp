#include "spi.h"
#include "system.h"

#include "SDCard.h"

#include "wdt.h"

// ======================================================================
//static const SPIExt_Index spidevSDFlash = (SPIExt_Index)(SPIExt_DevSDCard|SPIExt_Make_ClkDiv(2)|SPIExt_CPolHigh|SPIExt_CPha2Edge);
#define spidevSDCard	spi_dev_SDCard
// ======================================================================
#define SELECT_CARD     SPI_Switch_CS(spidevSDCard, true);
#define DESELECT_CARD   SPI_Switch_CS(spidevSDCard, false);
// ======================================================================

CSDCard::CSDCard():
  CLogObject(Log_SDCard),
  mIsInit(false)
{

}

// ======================================================================
//  public
// ======================================================================

/**
  * @brief  Инициализация SD карты
  * @param
  * @retval 1: в случае упешной инициализации
  */
int CSDCard::init()
{
  int res;

  // инициализируем шину SPI
  if (!this->powerUp()) {
    return -__LINE__;
  }

  // отправляем синхропосылку
  if ((res = this->sendSyncMess()) < 0) {
    return res;
  }

  Delay_ms(5);

  SELECT_CARD

  // idle mode
  if ((res = this->setIdleMode()) < 0) {
    DESELECT_CARD
    return res;
  }

  // check SD version
  if ((res = this->checkSDVer()) < 0) {
    DESELECT_CARD
    return res;
  }

  // initialize card and send host supports SDHC
  int cnt = 0;
  while (this->cardACmd(ACMD41, 0X40000000) != R1_READY_STATE) {
    cnt++;
    WDT_Restart();
  }

  // if SD2 read OCR register to check for SDHC card
  if (this->cardCmd(CMD58, 0)) {
    DESELECT_CARD
    return -__LINE__;
  }

  u8 buf[4];
  SPI_ReadBuffer(spidevSDCard, buf, _dim(buf));

  DESELECT_CARD

  if ((res = this->readCSD(&mCSD)) < 0) {
    DESELECT_CARD
    return -__LINE__;
  }

  mIsInit = true;
  return 1;
}

/**
  * @brief  Выдает кол-во блоков
  * @param
  * @retval
  */
u32 CSDCard::getBlockCount()
{
  if (!mIsInit) {
    return 0xFFFFFFFF;
  }

  u32 count = ((u32) mCSD.c_size_high << 16)
      | (mCSD.c_size_mid << 8) | mCSD.c_size_low;;

  count *= 1024;
  return count;
}

/**
  * @brief  Читаем блок данных
  * @param
  * @retval
  */
int CSDCard::readBlock(u8 *data, u32 address)
{
  SELECT_CARD

  if (this->cardCmd(CMD17, address)) {
    DESELECT_CARD
    return -__LINE__;
  }

  while (SPI_SendRecvByte(spidevSDCard, 0xFF) == 0xFF) {
    WDT_Restart();
  }

//	SPI_ReadBuffer(spidevSDCard, data, 512); // БЕЗ DMA!!!
	u8 buf[dataBlockSize];
	memset(buf, 0xFF, dataBlockSize);
	SPI_SendRecv(spidevSDCard, buf, data, dataBlockSize, false);

  SPI_SendRecvByte(spidevSDCard, 0xFF);
  SPI_SendRecvByte(spidevSDCard, 0xFF);

  DESELECT_CARD
  return 1;
}


// ======================================================================

// ======================================================================
//  protected
// ======================================================================

/**
  * @brief  Включение SPI шины для SD карты
  * @param
  * @retval
  */
bool CSDCard::powerUp()
{
  if ( SPI_IsInit(spidevSDCard) ) return true;
  if ( SPI_Init2(spidevSDCard) < 0 ) return false;
  return true;
}

// ======================================================================

// ======================================================================
//  private
// ======================================================================

// ======================================================================

/**
  * @brief  Ожидаем готовности SD карты
  * @param
  * @retval
  */
void CSDCard::waitUntilReady()
{
  u8 res = 0x00;
  while (res != 0xFF) {
    res = SPI_SendRecvByte(spidevSDCard, 0xFF);
    WDT_Restart();
  }
}

/**
  * @brief  Отправка команды на карту
  * @param
  * @retval 0> - успешно. 0< - номер строки, где была ошибка
  */
u8 CSDCard::cardCmd(u8 cmd, u32 arg)
{
  u8 cmdBuf[lenCmdRequest];
  cmdBuf[0] = (u8)(cmd | 0x40);
  cmdBuf[1] = (u8)(arg >> 24);
  cmdBuf[2] = (u8)(arg >> 16);
  cmdBuf[3] = (u8)(arg >> 8);
  cmdBuf[4] = (u8)(arg & 0xFF);
  cmdBuf[5] = (u8)(0xFF);

  if (cmd == CMD0) {
    cmdBuf[5] = 0x95;
  } else if (cmd == CMD8) {
    cmdBuf[5] = 0x87;
  }

  SELECT_CARD
  this->waitUntilReady();

  SPI_SendBuffer(spidevSDCard, cmdBuf, _dim(cmdBuf));

  int cnt = 0;
  u8 rsp;

  do {
    rsp = SPI_SendRecvByte(spidevSDCard, 0xFF);
  } while (rsp & 0x80 && cnt++ < countTryCardCmd);

  if (cnt >= countTryCardCmd) {
    Error("Error wait count: %d", -__LINE__);
    return -__LINE__;
  }

  return rsp;
}

/**
  * @brief  Отправка ACMD команды на SD карту
  * @param
  * @retval
  */
u8 CSDCard::cardACmd(u8 cmd, u32 arg)
{
  this->cardCmd(CMD55, 0);
  return this->cardCmd(cmd, arg);
}

/**
  * @brief  Читаем регистр SD карты
  * @param
  * @retval
  */
int CSDCard::readRegister(u8 cmd, u8 *buf)
{
  if (this->cardCmd(cmd, 0)) {
    DESELECT_CARD
    return -__LINE__;
  }

  while (SPI_SendRecvByte(spidevSDCard, 0xFF) == 0xFF) {
    WDT_Restart();
  }
  SPI_ReadBuffer(spidevSDCard, buf, 16);

  SPI_SendRecvByte(spidevSDCard, 0xFF);
  SPI_SendRecvByte(spidevSDCard, 0xFF);

  DESELECT_CARD
  return 1;
}

/**
  * @brief  Отправляем синхро посылку
  * @param
  * @retval
  */
int CSDCard::sendSyncMess()
{
  DESELECT_CARD

  u8 syncMessTx[lenSyncMess];
  memset(syncMessTx, 0xFF, _dim(syncMessTx));

  if (SPI_SendRecv(spidevSDCard, syncMessTx, NULL, _dim(syncMessTx), false) < 0) {
    return -__LINE__;
  }

  return 1;
}

/**
  * @brief  Перевод в режим Idle
  * @param
  * @retval
  */
int CSDCard::setIdleMode()
{
  if (this->cardCmd(CMD0, 0) != R1_IDLE_STATE) {
    return -__LINE__;
  }

  return 1;
}

/**
  * @brief  Проверка версии SD карты, поддерживаемая версия 2
  * @param
  * @retval
  */
int CSDCard::checkSDVer()
{
  if (this->cardCmd(CMD8, 0x1AA) & R1_ILLEGAL_COMMAND) {
    DESELECT_CARD
    return -__LINE__;
  } else {
    u8 buf[4];
    SPI_ReadBuffer(spidevSDCard, buf, _dim(buf));
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01 || buf[3] != 0xAA) {
      return -__LINE__;
    }
  }

  return 1;
}

/**
  * @brief  Читает регистр CSD
  * @param
  * @retval
  */
int CSDCard::readCSD(CSDCard::SCSDV2 *csd)
{
  return this->readRegister(CMD9, (u8*)csd);
}

// ======================================================================
