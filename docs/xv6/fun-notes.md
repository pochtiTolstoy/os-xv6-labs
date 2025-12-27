# XV6: System Calls и User-Space Stubs

## Идея system call

* User-программа **не вызывает ядро напрямую**.
* Любой вызов в ядро делается через **trap (`ecall`)**, CPU переключается в **kernel mode**.
* Ядро выполняет нужную функцию (`sys_*`), возвращает результат в user-space.

---

## Stub-функции в `usys.S`

* Stub = **трамплин из user space в ядро**.
* Пример `sleep`:

```asm
.global sleep
sleep:
    li a7, SYS_sleep   # номер системного вызова
    ecall              # trap в ядро
    ret
```

* `a7` — регистр, где xv6 хранит номер syscall.
* `ecall` — переход в kernel mode.
* `ret` — возврат в user space.

> В C это выглядит как обычная функция: `sleep(100);`

---

## Связывание stub с user-программой

1. В `user/user.h` объявляем прототип:

```c
int sleep(int ticks);
```

2. Линкер связывает вызов `sleep()` с stub в `usys.S`.
3. Аргументы передаются через регистры (RISC-V: a0, a1, …), результат возвращается через a0.

---

## Реализация в ядре

* В ядре есть `sys_sleep()`:

```c
uint64 sys_sleep(void) {
    int n;
    argint(0, &n);         // получить аргумент из user-space
    acquire(&tickslock);
    uint ticks0 = ticks;
    while(ticks - ticks0 < n) {
        if(killed(myproc())) {
            release(&tickslock);
            return -1;
        }
        sleep(&ticks, &tickslock); // блокируем процесс на тики
    }
    release(&tickslock);
    return 0;
}
```

* **Внутренний `sleep(void*, spinlock*)`** — kernel-level, блокирует процесс, отдаёт CPU, ждёт `wakeup()`.

---

## Генерация stubs автоматически

* Скрипт `usys.pl` создаёт все stubs для системных вызовов:

```perl
entry("fork");
entry("exit");
entry("sleep");
...
```

* Экономит ручное написание 20+ функций.

---

## Добавление нового syscall

1. Добавляем функцию `sys_mycall()` в ядро.
2. Добавляем номер в `kernel/syscall.h`.
3. Добавляем в таблицу `syscalls[]` в `kernel/syscall.c`.
4. Создаём stub в `usys.S` (или через perl-скрипт).
5. Добавляем прототип в `user/user.h`.
   Теперь user-программа может вызывать `mycall()` без написания C-кода в user space.

---

## Схема вызова

```
User-space: sleep(n)
        │
        ▼
usys.S stub (li a7, SYS_sleep; ecall; ret)
        │
        ▼
Kernel: syscall() dispatcher
        │
        ▼
sys_sleep() → kernel sleep(&ticks)
        │
timer interrupt → wakeup(&ticks)
        │
        ▼
sys_sleep() возвращает
        │
        ▼
User-space: продолжение выполнения
```

---