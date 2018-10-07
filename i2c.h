#ifndef I2C_H
#define I2C_H

#define TWI_TWBR 16                                  //Делитель скорости шины
#define WR                0                           //Запись данных
#define RD                1                           //Чтение данных
#define I2C_ACK               1                           //посылать подтверждение приема (не последний байт)
#define I2C_NOT_ACK           0                           //не подтверждать прием (последний байт)

//General Master staus codes
#define START             0x08                        //START has been transmitted
#define REP_START         0x10                        //Repeated START has been
                                                      //transmitted
//Master Transmitter status codes
#define MTX_ADR_ACK       0x18                        //SLA+W has been tramsmitted
                                                      //and ACK received
#define MTX_ADR_NACK      0x20                        //SLA+W has been tramsmitted
                                                      //and NACK received
#define MTX_DATA_ACK      0x28                        //Data byte has been tramsmitted
                                                      //and ACK received
#define MTX_DATA_NACK     0x30                        //Data byte has been tramsmitted
                                                      //and NACK received
#define MTX_ARB_LOST      0x38                        //Arbitration lost in SLA+W or
                                                      //data bytes
//Master Receiver status codes
#define MRX_ARB_LOST      0x38                        //Arbitration lost in SLA+R or
                                                      //NACK bit
#define MRX_ADR_ACK       0x40                        //SLA+R has been tramsmitted
                                                      //and ACK received
#define MRX_ADR_NACK      0x48                        //SLA+R has been tramsmitted
                                                      //and NACK received
#define MRX_DATA_ACK      0x50                        //Data byte has been received
                                                      //and ACK tramsmitted
#define MRX_DATA_NACK     0x58                        //Data byte has been received
                                                      //and NACK tramsmitted



void i2c_poll(void);

extern int i2c_time;
#define I2C_TIME 8192

#endif // I2C_H
