/**
  * @version
  * @brief    Класс для работы с SD картой
  */
#ifndef SDCARD_H
#define SDCARD_H

#ifdef USE_SD_CARD

#include "LogObject.h"

class CSDCard : public CLogObject
{

public:
  CSDCard();

  bool isInit() const {return mIsInit;}
  int init();
  int getBlockSize() const {return dataBlockSize;}
  u32 getBlockCount();

  int readBlock(u8 *data, u32 address);
	int writeBlock(const u8 *data, u32 address);

protected:

  struct SCSDV2 {
          // byte 0
          unsigned reserved1 :6;
          unsigned csd_ver :2;
          // byte 1
          uint8_t taac;
          // byte 2
          uint8_t nsac;
          // byte 3
          uint8_t tran_speed;
          // byte 4
          uint8_t ccc_high;
          // byte 5
          unsigned read_bl_len :4;
          unsigned ccc_low :4;
          // byte 6
          unsigned reserved2 :4;
          unsigned dsr_imp :1;
          unsigned read_blk_misalign :1;
          unsigned write_blk_misalign :1;
          unsigned read_bl_partial :1;
          // byte 7
          unsigned reserved3 :2;
          unsigned c_size_high :6;
          // byte 8
          uint8_t c_size_mid;
          // byte 9
          uint8_t c_size_low;
          // byte 10
          unsigned sector_size_high :6;
          unsigned erase_blk_en :1;
          unsigned reserved4 :1;
          // byte 11
          unsigned wp_grp_size :7;
          unsigned sector_size_low :1;
          // byte 12
          unsigned write_bl_len_high :2;
          unsigned r2w_factor :3;
          unsigned reserved5 :2;
          unsigned wp_grp_enable :1;
          // byte 13
          unsigned reserved6 :5;
          unsigned write_partial :1;
          unsigned write_bl_len_low :2;
          // byte 14
          unsigned reserved7 :2;
          unsigned file_format :2;
          unsigned tmp_write_protect :1;
          unsigned perm_write_protect :1;
          unsigned copy :1;
          unsigned file_format_grp :1;
          // byte 15
          unsigned always1 :1;
          unsigned crc :7;
  };

  // список команд
  enum {
    CMD0 = 0X00,	/** GO_IDLE_STATE - init card in spi mode if CS low */
    CMD8 = 0X08,	/** SEND_IF_COND - verify SD Memory Card interface operating condition.*/
    CMD9 = 0X09,	/** SEND_CSD - read the Card Specific Data (CSD register) */
    CMD10 = 0X0A,	/** SEND_CID - read the card identification information (CID register) */
    CMD13 = 0X0D,	/** SEND_STATUS - read the card status register */
    CMD17 = 0X11,	/** READ_BLOCK - read a single data block from the card */
    CMD24 = 0X18,	/** WRITE_BLOCK - write a single data block to the card */
    CMD25 = 0X19,	/** WRITE_MULTIPLE_BLOCK - write blocks of data until a STOP_TRANSMISSION */
    CMD32 = 0X20,	/** ERASE_WR_BLK_START - sets the address of the first block to be erased */
    CMD33 = 0X21,	/** ERASE_WR_BLK_END - sets the address of the last block of the continuous range to be erased*/
    CMD38 = 0X26,	/** ERASE - erase all previously selected blocks */
    CMD55 = 0X37,	/** APP_CMD - escape for application specific command */
    CMD58 = 0X3A,	/** READ_OCR - read the OCR register of a card */
    ACMD23 = 0X17,	/** SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be pre-erased before writing */
    ACMD41 = 0X29,	/** SD_SEND_OP_COMD - Sends host capacity support information and activates the card's initialization process */
  };

  enum {
    R1_READY_STATE = 0X00,	/** status for card in the ready state */
    R1_IDLE_STATE = 0X01,	/** status for card in the idle state */
    R1_ILLEGAL_COMMAND = 0X04,	/** status bit for illegal command */
    DATA_START_BLOCK = 0XFE,	/** start data token for read or write single block*/
    STOP_TRAN_TOKEN = 0XFD,	/** stop token for write multiple blocks*/
    WRITE_MULTIPLE_TOKEN = 0XFC,	/** start data token for write multiple blocks*/
    DATA_RES_MASK = 0X1F,	/** mask for data response tokens after a write block operation */
    DATA_RES_ACCEPTED = 0X05,	/** write data accepted token */
  };

  enum {
    dataBlockSize = 512,
    countTryCardCmd = 20,
    lenSyncMess = 10, // длина синхропосылки
    lenCmdRequest = 6, // длина команды для SD карты
  };

protected:
  bool mIsInit;
  SCSDV2 mCSD;

protected:
  bool powerUp();

private:
  void waitUntilReady();
	void waitUntilDone();
  u8 cardCmd(u8 cmd, u32 arg);
  u8 cardACmd(u8 cmd, u32 arg);
  int readRegister(u8 cmd, u8 *buf);

  int sendSyncMess();
  int setIdleMode();
  int checkSDVer();
  int readCSD(SCSDV2 *csd);

};


#endif // USE_SD_CARD
#endif // SDCARD_H
