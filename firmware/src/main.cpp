#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stdint.h>

// ============================================================
// CONFIG
// ============================================================

#define BAUD_RATE          115200UL
#define MPU_ADDR           0x68
#define SAMPLE_RATE_HZ     1000
#define TWI_TIMEOUT_CYCLES 4000UL
#define MPU_RETRY_TICKS    100

volatile uint8_t sample_count = 0;

#define DEBUG_PIN_PORT PORTB
#define DEBUG_PIN_DDR  DDRB
#define DEBUG_PIN_BIT  PB5   // D13

// ============================================================
// UART
// ============================================================

static uint32_t u32_abs_diff(uint32_t a, uint32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

static void uart_init(void)
{
    uint16_t ubrr_u2x = (uint16_t)(((F_CPU + (BAUD_RATE * 4UL)) / (BAUD_RATE * 8UL)) - 1UL);
    uint16_t ubrr_nrm = (uint16_t)(((F_CPU + (BAUD_RATE * 8UL)) / (BAUD_RATE * 16UL)) - 1UL);
    uint32_t baud_u2x = F_CPU / (8UL * ((uint32_t)ubrr_u2x + 1UL));
    uint32_t baud_nrm = F_CPU / (16UL * ((uint32_t)ubrr_nrm + 1UL));

    if (u32_abs_diff(baud_u2x, BAUD_RATE) <= u32_abs_diff(baud_nrm, BAUD_RATE))
    {
        UCSR0A = (1 << U2X0);
        UBRR0H = (ubrr_u2x >> 8);
        UBRR0L = ubrr_u2x;
    }
    else
    {
        UCSR0A = 0;
        UBRR0H = (ubrr_nrm >> 8);
        UBRR0L = ubrr_nrm;
    }

    // 8N1 TX only
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

static void uart_write_byte(uint8_t data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

static void uart_write_buffer(const uint8_t* data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
        uart_write_byte(data[i]);
}

// ============================================================
// TWI (I2C)
// ============================================================

static void twi_init(void)
{
    TWSR = 0x00;
    TWBR = 12; // 400 kHz @ 16MHz
    TWCR = (1 << TWEN);
}

static uint8_t twi_wait_int(void)
{
    uint32_t timeout = TWI_TIMEOUT_CYCLES;

    while (!(TWCR & (1 << TWINT)))
    {
        if (--timeout == 0)
            return 0;
    }
    return 1;
}

static uint8_t twi_start(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    if (!twi_wait_int())
        return 0;

    return (TW_STATUS == TW_START) || (TW_STATUS == TW_REP_START);
}

static void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

static uint8_t twi_write_expect(uint8_t data, uint8_t expected_status)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    if (!twi_wait_int())
        return 0;

    return TW_STATUS == expected_status;
}

static uint8_t twi_read_ack(uint8_t* data)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    if (!twi_wait_int())
        return 0;
    if (TW_STATUS != TW_MR_DATA_ACK)
        return 0;

    *data = TWDR;
    return 1;
}

static uint8_t twi_read_nack(uint8_t* data)
{
    TWCR = (1 << TWINT) | (1 << TWEN);
    if (!twi_wait_int())
        return 0;
    if (TW_STATUS != TW_MR_DATA_NACK)
        return 0;

    *data = TWDR;
    return 1;
}

// ============================================================
// MPU6050
// ============================================================

static uint8_t mpu_write(uint8_t reg, uint8_t data)
{
    if (!twi_start())
        goto fail;
    if (!twi_write_expect((MPU_ADDR << 1) | 0, TW_MT_SLA_ACK))
        goto fail;
    if (!twi_write_expect(reg, TW_MT_DATA_ACK))
        goto fail;
    if (!twi_write_expect(data, TW_MT_DATA_ACK))
        goto fail;

    twi_stop();
    return 1;

fail:
    twi_stop();
    return 0;
}

static uint8_t mpu_read_accel(int16_t* ax, int16_t* ay, int16_t* az)
{
    uint8_t xh, xl, yh, yl, zh, zl;

    if (!twi_start())
        goto fail;
    if (!twi_write_expect((MPU_ADDR << 1) | 0, TW_MT_SLA_ACK))
        goto fail;
    if (!twi_write_expect(0x3B, TW_MT_DATA_ACK)) // ACCEL_XOUT_H
        goto fail;
    if (!twi_start())
        goto fail;
    if (!twi_write_expect((MPU_ADDR << 1) | 1, TW_MR_SLA_ACK))
        goto fail;

    if (!twi_read_ack(&xh))
        goto fail;
    if (!twi_read_ack(&xl))
        goto fail;
    if (!twi_read_ack(&yh))
        goto fail;
    if (!twi_read_ack(&yl))
        goto fail;
    if (!twi_read_ack(&zh))
        goto fail;
    if (!twi_read_nack(&zl))
        goto fail;

    twi_stop();

    *ax = (int16_t)((xh << 8) | xl);
    *ay = (int16_t)((yh << 8) | yl);
    *az = (int16_t)((zh << 8) | zl);
    return 1;

fail:
    twi_stop();
    return 0;
}

static uint8_t mpu_init(void)
{
    if (!mpu_write(0x6B, 0x00)) // PWR_MGMT_1
        return 0;
    if (!mpu_write(0x19, 0x00)) // SMPLRT_DIV
        return 0;
    if (!mpu_write(0x1A, 0x03)) // DLPF 44Hz
        return 0;
    if (!mpu_write(0x1C, 0x00)) // +/-2G
        return 0;

    return 1;
}

// ============================================================
// TIMER 1kHz
// ============================================================

static void timer1_init(void)
{
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS10); // CTC, no prescaler
    OCR1A = (F_CPU / SAMPLE_RATE_HZ) - 1;
    TIMSK1 = (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect)
{
    if (sample_count < 0xFF)
        sample_count++;

    DEBUG_PIN_PORT ^= (1 << DEBUG_PIN_BIT); // toggle D13
}

// ============================================================
// MAIN
// ============================================================

int main(void)
{
    uart_init();
    twi_init();
    timer1_init();

    DEBUG_PIN_DDR |= (1 << DEBUG_PIN_BIT); // D13 output

    sei();

    int16_t ax, ay, az;
    uint8_t mpu_ready = mpu_init();
    uint8_t mpu_retry_ticks = 0;

    while (1)
    {
        uint8_t pending;

        cli();
        pending = sample_count;
        sample_count = 0;
        
        sei();

        if (!pending)
            continue;

        while (pending--)
        {
            if (!mpu_ready)
            {
                if (mpu_retry_ticks == 0)
                {
                    mpu_ready = mpu_init();
                    mpu_retry_ticks = MPU_RETRY_TICKS;
                }
                else
                {
                    mpu_retry_ticks--;
                }

                ax = 0x7FFF;
                ay = 0x7FFF;
                az = 0x7FFF;
            }
            else if (!mpu_read_accel(&ax, &ay, &az))
            {
                mpu_ready = 0;
                mpu_retry_ticks = MPU_RETRY_TICKS;
                ax = 0x7FFF;
                ay = 0x7FFF;
                az = 0x7FFF;
            }

            uart_write_buffer((const uint8_t*)&ax, 2);
            uart_write_buffer((const uint8_t*)&ay, 2);
            uart_write_buffer((const uint8_t*)&az, 2);
        }
    }
}
