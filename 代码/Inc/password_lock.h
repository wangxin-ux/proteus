#ifndef PASSWORD_LOCK_H
#define PASSWORD_LOCK_H

/* Six digits; changes last until reset. No nonvolatile storage is fitted. */
#define LOCK_PASSWORD_LEN 6
#define LOCK_OPEN_MS      3000U
#define LOCK_ALARM_MS     30000U

void Lock_Init(void);
void Lock_HandleKey(unsigned char key);
void Lock_Task(void);

#endif
