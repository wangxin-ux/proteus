/* Electronic password lock application. All timeouts use the HAL millisecond
 * tick. PA1 drives an active-high buzzer only during the alarm. */
#include "main.h"
#include "lcd1602.h"
#include "password_lock.h"
#include "buzzer.h"
#include <string.h>

typedef enum {
    LOCK_MENU, LOCK_UNLOCK, LOCK_OLD, LOCK_NEW, LOCK_CONFIRM,
    LOCK_OPEN, LOCK_NOTICE, LOCK_ALARM
} LockState;

static LockState state;
static LockState notice_next;
static char password[LOCK_PASSWORD_LEN + 1];
static char input[LOCK_PASSWORD_LEN + 1];
static char candidate[LOCK_PASSWORD_LEN + 1];
static unsigned char input_count;
static unsigned char failures;
static uint32_t started;
static uint32_t notice_ms;
static unsigned char alarm_second;

/* Pad each line once, with no clear-screen flicker or stale characters. */
static void Show(const char *top, const char *bottom)
{
    char line[17];
    unsigned char row, i;
    const char *text;
    for (row = 0; row < 2; row++) {
        text = row ? bottom : top;
        memset(line, ' ', 16);
        line[16] = '\0';
        for (i = 0; i < 16 && text[i]; i++) line[i] = text[i];
        LCD_write_string(0, row, line);
    }
}

static void ClearInput(void)
{
    input_count = 0;
    memset(input, 0, sizeof(input));
}

static void ShowInput(void)
{
    char line[17] = "PWD:            ";
    const char *title = "ENTER PASSWORD";
    unsigned char i;
    if (state == LOCK_OLD) title = "VERIFY OLD PWD";
    if (state == LOCK_NEW) title = "NEW PWD:6 DIGITS";
    if (state == LOCK_CONFIRM) title = "REPEAT NEW PWD";
    for (i = 0; i < input_count; i++) line[4 + i] = '*';
    Show(title, line);
}

static void Enter(LockState next)
{
    state = next;
    ClearInput();
    if (next == LOCK_MENU) {
        memset(candidate, 0, sizeof(candidate));
        Show("LOCKED A:OPEN", "B:CHANGE PWD");
    } else ShowInput();
}

static void Notice(const char *top, const char *bottom,
                   LockState next, uint32_t duration)
{
    state = LOCK_NOTICE;
    notice_next = next;
    notice_ms = duration;
    started = HAL_GetTick();
    Show(top, bottom);
}

static void WrongPassword(void)
{
    char line[] = "TRIES LEFT: 2";
    failures++;
    ClearInput();
    memset(candidate, 0, sizeof(candidate));
    if (failures >= 3) {
        Buzzer_On();
        state = LOCK_ALARM;
        started = HAL_GetTick();
        alarm_second = 255;
        Lock_Task();
    } else {
        line[12] = '0' + (3 - failures);
        Notice("PASSWORD ERROR", line, state, 1000U);
    }
}

void Lock_Init(void)
{
    memcpy(password, "123456", sizeof(password));
    failures = 0;
    Buzzer_Off();
    Enter(LOCK_MENU);
}

void Lock_HandleKey(unsigned char key)
{
    /* Menu/cancel/clear cannot bypass the alarm or reset failed attempts. */
    if (state == LOCK_ALARM || state == LOCK_NOTICE) return;
    if (state == LOCK_OPEN) {
        if (key == '*' || key == 'D') {
            Buzzer_Off();
            Enter(LOCK_MENU);
        }
        return;
    }
    if (key == '*') { Enter(LOCK_MENU); return; }
    if (state == LOCK_MENU) {
        if (key == 'A') Enter(LOCK_UNLOCK);
        else if (key == 'B') Enter(LOCK_OLD);
        return;
    }
    if (key >= '0' && key <= '9') {
        if (input_count < LOCK_PASSWORD_LEN) {
            input[input_count++] = key;
            input[input_count] = '\0';
            ShowInput();
        }
    } else if (key == 'C') {
        if (input_count) input[--input_count] = '\0';
        ShowInput();
    } else if (key == '#') {
        ClearInput();
        ShowInput();
    } else if (key == 'D') {
        if (input_count != LOCK_PASSWORD_LEN) {
            /* Incomplete entry is an editing error, not a failed password. */
            Notice("NEED 6 DIGITS", "C:DEL D:CONFIRM", state, 1000U);
            return;
        }
        if (state == LOCK_UNLOCK || state == LOCK_OLD) {
            if (strcmp(input, password) != 0) { WrongPassword(); return; }
            failures = 0;
            if (state == LOCK_OLD) Enter(LOCK_NEW);
            else {
                ClearInput();
                state = LOCK_OPEN;
                started = HAL_GetTick();
                Buzzer_Off();
                Show("PASSWORD OK", "OPEN 3s D:LOCK");
            }
        } else if (state == LOCK_NEW) {
            memcpy(candidate, input, sizeof(candidate));
            Enter(LOCK_CONFIRM);
        } else if (state == LOCK_CONFIRM) {
            if (strcmp(input, candidate) == 0) {
                memcpy(password, candidate, sizeof(password));
                memset(candidate, 0, sizeof(candidate));
                ClearInput();
                Notice("PASSWORD CHANGED", "LOCKED", LOCK_MENU, 1200U);
            } else {
                memset(candidate, 0, sizeof(candidate));
                ClearInput();
                Notice("PWD MISMATCH", "ENTER NEW AGAIN", LOCK_NEW, 1200U);
            }
        }
    }
}

void Lock_Task(void)
{
    uint32_t elapsed = (uint32_t)(HAL_GetTick() - started);
    unsigned char seconds;
    char line[] = "RETRY IN 30s";
    if (state == LOCK_OPEN && elapsed >= LOCK_OPEN_MS) {
        Buzzer_Off();
        Enter(LOCK_MENU);
    } else if (state == LOCK_NOTICE && elapsed >= notice_ms) {
        /* Keep partially entered digits for the length warning only. */
        state = notice_next;
        if (state == LOCK_MENU) Enter(state);
        else ShowInput();
    } else if (state == LOCK_ALARM) {
        if (elapsed >= LOCK_ALARM_MS) {
            Buzzer_Off();
            failures = 0;
            Enter(LOCK_MENU);
        } else {
            seconds = (unsigned char)((LOCK_ALARM_MS - elapsed + 999U) / 1000U);
            if (seconds != alarm_second) {
                alarm_second = seconds;
                line[9] = '0' + seconds / 10;
                line[10] = '0' + seconds % 10;
                Show((seconds & 1) ? "!!! ALARM !!!" : "3 WRONG PASSWORD", line);
            }
        }
    }
}
