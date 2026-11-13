# Индивидуальное задание №3

Исполнитель: Вылкова Амалия Алексеевна


Группа: БПИ253

Вариант: 41. Разводной мост

---

## 1. Условие задачи

Через судоходный канал проходит разводной автомобильный мост. По дороге прибывают автомобили, а по каналу с двух направлений подходят суда. В обычном состоянии пролёт опущен и открыт для автомобильного движения. Для пропуска судов оператор должен остановить транспорт, закрыть шлагбаумы, дождаться освобождения моста, поднять пролёт и разрешить проход. После прохождения сформированной группы судов пролёт опускается, фиксируется, шлагбаумы открываются и движение автомобилей возобновляется. Подъём и опускание занимают определённое время. Экстренный автомобиль или приоритетное судно может влиять на расписание, но не отменяет обязательную последовательность безопасных состояний.
Требуется разработать консольное приложение, моделирующее работу разводного моста.

**Входные параметры:**

- интенсивность автомобильного и судового потоков;
- вместимость очередей;
- время проезда автомобиля и прохода судна;
- время закрытия шлагбаумов, подъёма и опускания;
- максимальное время ожидания каждого потока;
- стратегия формирования групп судов;
- правила обслуживания приоритетных участников.

**Отображаемые события:**

Программа отображает прибытие, постановку в очередь, закрытие и открытие шлагбаумов, освобождение дорожного полотна, подъём и опускание пролёта, разрешение движения, въезд автомобиля и проход судна.

**Инварианты:**

- подъём не начинается, пока на мосту находится автомобиль;
- автомобиль не въезжает при закрытом шлагбауме или поднятом пролёте;
- судно не проходит под закрытым мостом;
- противоположные разрешения автомобильному и судовому потокам не действуют одновременно;
- переходы между состояниями выполняются в установленном порядке.

**Завершение:**

Моделирование завершается после обслуживания заданного числа участников либо по окончании смены с выводом оставшихся очередей.
Оператор, механизм моста, автомобили и суда являются самостоятельными участниками модели.

---

## 2. Сценарий: сущности → процессы

Программа построена как один родительский процесс, который форкает несколько дочерних. Все дочерние процессы взаимодействуют через POSIX
разделяемую память и набор именованных POSIX семафоров.

| Предметная сущность | Процесс | Поведение |
|---|---|---|
| Оператор | `operator_process` | Спит на `g_event`. При пробуждении под `g_mutex` читает состояние моста и очередей и, если возможно, инициирует следующую фазу автомата моста, заполняя `mechanism_cmd`. Если действие выполнено - будит механизм через `g_mech`. |
| Механизм моста | `mechanism_process` | Спит на `g_mech`. При получении команды под `g_mutex` забирает её, спит `cmd_duration` секунд (разбито на шаги по 100 мс для быстрой реакции на остановку), затем под `g_mutex` вызывает `bridge_finish_operation` и будит оператора через `g_event`. |
| Генератор автомобилей | `car_generator` | По экспоненциальному интервалу раз в `lambda_car` порождает автомобиль: под `g_mutex` занимает слот в `car_slots[]`, присваивает `id`, делает `fork()`. Дочерний процесс моделирует один автомобиль. После каждой итерации будит оператора через `g_event`. |
| Один автомобиль | `car_process` | Ждёт разрешения (`bridge.state == ROAD_OPEN && permission == CARS`), под `g_mutex` инкрементирует `cars_on_bridge`, спит `car_cross_time`, под `g_mutex` декрементирует `cars_on_bridge`, пишет `CAR_EXIT`, освобождает слот. Будит оператора через `g_event` - оператор узнаёт, что мост освободился. |
| Генератор судов сверху | `ship_generator(SHIP_UP)` | Аналогично генератору автомобилей, но с интенсивностью `lambda_up`, шансом приоритета `prio_prob_ship` и записью в кольцевые очереди `ships_up_normal` / `ships_up_priority`. |
| Генератор судов снизу | `ship_generator(SHIP_DOWN)` | Симметрично, направление `down`. |
| Одно судно | `ship_process` | Ждёт, когда попадёт в группу и пролёт окажется поднят (`state == SHIPS_PASSING && me->state == SHIP_IN_GROUP`), под `g_mutex` переходит в `SHIP_PASSING`, спит `ship_pass_time`, затем под `g_mutex` пишет `SHIP_EXIT`, уменьшает `ships_passing`, увеличивает `ships_served`. Будит оператора через `g_event`. |
| Таймер окончания смены (опционально) | `shift_timer_main` | Если задан `--shift-end > 0`, спит `shift_end` секунд, затем под `g_mutex` ставит `stop = 1` и будит оператора/механизм. |
| Контроль `max_served` (опционально) | `max_served_watchdog` | Раз в 500 мс под `g_mutex` проверяет `served_total >= max_served`; при достижении ставит `stop = 1`. |
| Родитель | `main` | Разбирает аргументы, создаёт SHM и семафоры, форкает всех детей, ждёт `stop`, затем шлёт `SIGTERM`, собирает всех через `waitpid`, печатает сводку и вызывает `ipc_cleanup()` (shm_unlink + sem_unlink). |

**Модель времени.** Здесь время реальное: `clock_gettime(CLOCK_MONOTONIC)` в каждом процессе считает `sim_time_now()` от момента старта симуляции. Тайминги - через `nanosleep`. Длительности в конфиге задают *реальные* секунды.

**Модель очередей машин.** Каждый автомобиль - отдельный процесс; он «стоит в очереди» неявно (поллинг состояния моста с шагом 50 мс).
Вместимость «очереди машин» фактически определяется константой `MAX_CARS = 256` (слот `car_slots`); при исчерпании - `cars_rejected++`.
Очереди судов (`normal` / `priority` для `up` / `down`) - настоящие кольцевые буферы в SHM с параметром `--ship-cap` (по умолч. 16).

---

## 3. Объекты IPC

### 3.1. Разделяемая память

Одна именованная область POSIX SHM: `/bridge_shm_v41`. Структура `SharedState` (в `shared.h`) содержит:

- `Config cfg` - конфигурация (копируется родителем один раз);
- `Bridge bridge` - состояние конечного автомата;
- `CarDesc car_slots[MAX_CARS]` - дескрипторы автомобилей (id, priority, state, arrival_time, pid, used);
- кольцевые буферы судов - `RingHeader` + `int idx[]` для normal/priority × up/down;
- `ship_group[]` и `ship_group_size` - текущая группа под поднятым пролётом;
- статистика (`served_total`, `cars_served`, `ships_served`, `cars_rejected`, `ships_rejected`);
- счётчики `next_car_id` / `next_ship_id`;
- `MechCmd mechanism_cmd` - команда оператора механизму;
- `volatile sig_atomic_t stop` - общий флаг остановки.

### 3.2. Семафоры (именованные POSIX)

| Имя | Стартовое значение | Назначение |
|---|---|---|
| `/bridge_sem_mutex_v41` | 1 | Защита всей `SharedState`. Все чтения/записи - под этим семафором. Обёрнут в `mutex_lock()` / `mutex_unlock()` (EINTR-safe). |
| `/bridge_sem_log_v41` | 1 | Защита вывода в `stdout` и `sim.log`. Строки не рвутся между процессами. Обёрнут в `sem_wait` с retry на EINTR. |
| `/bridge_sem_event_v41` | 0 | Сигнал «состояние изменилось» для оператора. `sem_post` делают: генераторы, `car_process` (после `CAR_EXIT`), `ship_process` (после `SHIP_EXIT`), механизм (после операции), `shift_timer`, `max_served_watchdog`. |
| `/bridge_sem_mech_v41` | 0 | Сигнал «есть команда» для механизма. `sem_post` делает только оператор после успешного `operator_review()`. |

Все семафоры именованные, создаются в `ipc_setup()` с `O_CREAT` и открываются в детях через наследование (после `fork()` они не
пересоздаются, а наследуются как указатели на уже отмапленные семафоры). `ipc_cleanup()` делает `sem_close` + `sem_unlink` для каждого
и `shm_unlink` для `/bridge_shm_v41`.

### 3.3. Схема взаимодействия

```
   car_generator ── fork ──> car_process ── sem_post(g_event)
        │                                         │
        │ sem_post(g_event)                       │
        ▼                                         │
   ship_generator_up   ── fork ──> ship_process ──┤
   ship_generator_down ── fork ──> ship_process ──┤
                                                  ▼
                                           ┌────────────┐
   mechanism_process ◄── sem_post(g_mech) ─│  operator  │
        │                                  │  process   │
        └── sem_post(g_event) ────────────►└────────────┘
                                               ▲
   shift_timer / watchdog ── sem_post ─────────┘
```

Строго одна пара «оператор ↔ механизм» общается командами через `mechanism_cmd` и `g_mech`/`g_event`. Все остальные процессы (генераторы,
автомобили, суда) только сообщают оператору о факте изменения через `g_event`.

---

## 4. Пользовательский интерфейс

Консольный. Строки пишутся одновременно в stdout и в файл `sim.log` (append, с защитой `g_log_mutex`). Родитель до `fork()` один раз открывает
`sim.log` с `O_TRUNC`, чтобы обнулить его; дети пишут `O_APPEND`.

В начале работы печатается конфигурация:

```
=== Конфигурация ===
Интенсивности:   car=0.500, ship_up=0.100, ship_down=0.100
Очереди:         cars=32, ships=16
...
====================
```

Далее - поток событий:

```
[t=  0.10] CAR_ARRIVAL id=1
[t=  0.10] OP: ROAD_OPEN -> CLOSE_BARRIER, cmd=1
[t=  0.20] MECH: begin CLOSE_BARRIER (0.10s)
[t=  0.30] MECH: CLOSE_BARRIER done, state=WAIT_ROAD_CLEAR
[t=  0.50] CAR_ENTER id=1
[t=  0.70] CAR_EXIT  id=1
[t=  1.00] SHIP_ARRIVAL id=1 dir=up
[t=  1.00] OP: WAIT_ROAD_CLEAR -> RAISE, cmd=2
...
```

Все ключевые события предметной области отображаются:
`CAR_ARRIVAL`, `CAR_ENTER`, `CAR_EXIT`, `SHIP_ARRIVAL`, `SHIP_ENTER`,
`SHIP_EXIT`, `CAR_REJECTED`, `SHIP_REJECTED`, `OP: <before> -> <after>`,
`MECH: begin <cmd>`, `MECH: <cmd> done, state=<...>`.

В конце печатается сводка:

```
=== Итог ===
Модельное время:        12.34
Обслужено:              cars=20, ships=8
Отказы (очередь полна): cars=0, ships=0
Осталось в очередях:    cars=0, ships_up=0, ships_down=0
```

---

## 5. Варианты запуска и завершения

### Запуск

```
./build/bridge [опции]
```

Опции:

```
--car-lambda, --ship-lambda-up, --ship-lambda-down   # интенсивности
--car-cap, --ship-cap                                # вместимости
--car-cross, --ship-pass                             # времена проезда/прохода
--barrier-close, --barrier-open, --raise, --lower    # времена механизма
--max-car-wait, --max-ship-wait                      # таймауты
--prio-prob-car, --prio-prob-ship                    # шансы приоритета
--group-strategy=fifo|priority --max-group           # стратегия групп судов
--max-served, --shift-end                            # условия завершения
--seed, --delay                                      # seed ГСЧ, задержка вывода
```

Все перечисленные параметры разбираются в `config_parse_args()`, сохраняются
в `Config` и печатаются в блоке `=== Конфигурация ===` при старте. Часть
параметров в текущей версии модели принимается, но на поведение не влияет:

- `--car-cap` - фактическая вместимость машин ограничена константой `MAX_CARS = 256`; очередь машин реализована поллингом состояния моста,
  а не через ring-буфер, поэтому `--car-cap` не меняет число принимаемых машин;
- `--max-car-wait`, `--max-ship-wait` - принимаются и печатаются, но таймауты ожидания в текущей версии не enforced: машина или судно ждут
  до фактического разрешения проезда/прохода либо до остановки симуляции;
- `--prio-prob-car` - вероятность приоритета сохраняется в `CarDesc.priority`, но при проезде через мост приоритет машины не учитывается (все машины
  ждут в порядке поллинга).

Остальные параметры (`--car-lambda`, `--ship-lambda-*`, `--ship-cap`, `--car-cross`, `--ship-pass`, `--barrier-close`, `--barrier-open`, `--raise`, `--lower`, `--prio-prob-ship`, `--group-strategy`, `--max-group`, `--max-served`, `--shift-end`, `--seed`, `--delay`) влияют на модель так, как описано в разделе 2.

Примеры:

```bash
# По умолчанию
./build/bridge

# Быстрая демонстрация
./build/bridge --raise 0.3 --lower 0.3 \
               --barrier-close 0.1 --barrier-open 0.1 \
               --car-cross 0.2 --ship-pass 0.3 --max-served 20

# Завершение по окончании смены
./build/bridge --shift-end 5.0

# Приоритетная стратегия формирования групп судов
./build/bridge --group-strategy priority --max-group 10

# Замедленный вывод (полезно для наблюдения в реальном времени)
./build/bridge --delay 0.2 --max-served 20
```

### Завершение

Программа завершается любым из трёх способов:

1. **Естественное по `--max-served N`** - `max_served_watchdog` ставит `stop = 1`, когда `served_total >= N`.
2. **По `--shift-end T`** - `shift_timer_main` спит `T` секунд, ставит `stop = 1`.
3. **По сигналу** - `SIGINT` (`Ctrl-C`) или `SIGTERM`. Обработчик выставляет `*g_flag = 1` в `shm->stop`. Родитель, увидев `stop`,
   делает `sem_post(g_event)` + `sem_post(g_mech)` (чтобы разбудить всех, кто висит в `sem_wait`), затем `kill(0, SIGTERM)` (прерывает
   `nanosleep`), затем `waitpid` со всеми потомками и `ipc_cleanup()`.

Во всех трёх случаях:

- после выхода родителя `/dev/shm/bridge_shm_v41`, `/dev/shm/sem.bridge_sem_*` удалены;
- зомби-процессов не остаётся (проверено через `pgrep -af '/build/bridge'`);
- в `sim.log` дописана итоговая сводка.

Обработка `EINTR` в критических точках (`mutex_lock`, `waitpid`, `logger_printf`, `sem_wait(g_event/g_mech)`) гарантирует, что сигнал не «сломает» семафоры и не оставит полуобновлённое состояние.

---

## 6. Результаты работы программы

### 6.1. Естественное завершение (по `--max-served`)

Команда:

```bash
./build/bridge --raise 0.3 --lower 0.3 \
               --barrier-close 0.1 --barrier-open 0.1 \
               --car-cross 0.2 --ship-pass 0.3 \
               --max-served 20
```

Фрагмент `sim.log`:

```
[t=   0.68] CAR_ARRIVAL id=1
[t=   0.68] CAR_ENTER id=1
[t=   0.88] CAR_EXIT  id=1
[t=   1.27] CAR_ARRIVAL id=2
[t=   1.28] CAR_ENTER id=2
[t=   1.48] CAR_EXIT  id=2
[t=   2.36] CAR_ARRIVAL id=3 (priority)
[t=   2.36] CAR_ENTER id=3
[t=   2.56] CAR_EXIT  id=3
[t=   3.82] CAR_ARRIVAL id=4
[t=   3.83] CAR_ENTER id=4
[t=   4.03] CAR_EXIT  id=4
[t=   5.73] CAR_ARRIVAL id=5
[t=   5.74] CAR_ENTER id=5
[t=   5.94] CAR_EXIT  id=5
[t=   6.90] CAR_ARRIVAL id=6
[t=   6.90] CAR_ENTER id=6
[t=   7.10] CAR_EXIT  id=6
[t=   9.07] CAR_ARRIVAL id=7
[t=   9.07] CAR_ENTER id=7
...
=== Итог ===
Модельное время:        27.61
Обслужено:              cars=17, ships=3
Отказы (очередь полна): cars=0, ships=0
Осталось в очередях:    cars=0, ships_up=0, ships_down=0
```


### 6.2. Завершение по `Ctrl-C`

Команда:

```bash
./build/bridge --car-lambda 0.1
# через несколько секунд нажат Ctrl-C
```

Результат:

```
pgrep -af /build/bridge
(пусто)
```

```
ls /dev/shm | grep bridge_
(пусто)
```

`sim.log`:

```
=== Итог ===
Модельное время:        1.78
Обслужено:              cars=0, ships=0
Отказы (очередь полна): cars=0, ships=0
Осталось в очередях:    cars=0, ships_up=0, ships_down=0
```

### 6.3. Завершение по `--shift-end 5.0`

Команда:

```bash
./build/bridge --shift-end 5.0 --car-lambda 0.5 --ship-lambda-up 0.2
```

```
...
=== Итог ===
Модельное время:        5.01
Обслужено:              cars=4, ships=0
Отказы (очередь полна): cars=0, ships=0
Осталось в очередях:    cars=0, ships_up=0, ships_down=0
```

### 6.4. Проверка стратегии `priority`

Команда:

```bash
./build/bridge --group-strategy priority --max-group 5 \
               --raise 0.3 --lower 0.3 \
               --ship-lambda-up 0.4 --ship-lambda-down 0.4 \
               --prio-prob-ship 0.5
```

Фрагмент `sim.log`. Суда входят на мост строго в порядке `ship_group[]`, сформированной оператором по
стратегии `PRIORITY`: сначала все приоритетные суда обоих направлений в порядке `arrival_time`, затем обычные. Это достигается тем, что
`operator_form_ship_group` присваивает каждому судну `group_id = 0, 1, 2, …`, а `ship_process` входит на мост только когда
`me->group_id == shm->bridge.next_in_group`, и сразу инкрементирует `next_in_group` под `g_mutex`:

```text
9:[t=   3.28] SHIP_ARRIVAL id=1 dir=up
12:[t=   3.44] SHIP_ARRIVAL id=2 dir=up (priority)
19:[t=   4.70] SHIP_ENTER id=2 dir=up
20:[t=   4.74] SHIP_ENTER id=1 dir=up
21:[t=   5.48] SHIP_ARRIVAL id=3 dir=down (priority)
28:[t=   7.81] SHIP_ARRIVAL id=4 dir=up (priority)
32:[t=   8.52] SHIP_ARRIVAL id=5 dir=up (priority)
41:[t=  10.38] SHIP_ARRIVAL id=6 dir=up (priority)
42:[t=  10.38] SHIP_ENTER id=3 dir=down
43:[t=  10.41] SHIP_ENTER id=4 dir=up
44:[t=  10.42] SHIP_ENTER id=5 dir=up
45:[t=  10.69] SHIP_ARRIVAL id=7 dir=up (priority)
46:[t=  11.72] SHIP_ARRIVAL id=8 dir=up
53:[t=  13.48] SHIP_ARRIVAL id=9 dir=up
57:[t=  13.88] SHIP_ARRIVAL id=10 dir=up (priority)
59:[t=  14.67] SHIP_ARRIVAL id=11 dir=up
67:[t=  16.04] SHIP_ENTER id=6 dir=up
68:[t=  16.05] SHIP_ENTER id=7 dir=up
69:[t=  16.09] SHIP_ENTER id=10 dir=up
70:[t=  16.13] SHIP_ENTER id=8 dir=up
71:[t=  16.14] SHIP_ENTER id=9 dir=up
```

В каждой группе видна приоритетная последовательность:

- группа 1: `id=2 (priority)` → `id=1 (normal)`;
- группа 2: `id=3, id=4, id=5` - все приоритетные, порядок по `arrival_time` (5.48 < 7.81 < 8.52);
- группа 3: `id=6, id=7, id=10` (приоритетные, 10.38 < 10.69 < 13.88), затем `id=8, id=9` (обычные, 11.72 < 13.48).


---

## 7. Комментарии и ограничения

- Время симуляции - реальное (`CLOCK_MONOTONIC`). Параметры из `--raise`, `--car-cross` и т. п. - реальные секунды.
- «Очередь машин» реализована через лимит одновременных процессов (`MAX_CARS = 256`); очередь судов - через ring-буферы на `--ship-cap`.
- При разбиении `nanosleep` в механизме на шаги по 100 мс реакция на
  `Ctrl-C` ограничена сверху ~100 мс.
- Обработчики сигналов в детях и родителе одинаковые (одна и та же `signals_install`), но указатель `g_flag` во всех процессах указывает на
  `shm->stop`, поэтому флаг общий.

---

## 8. Структура проекта

```
hw3-drawbridge/
├── CMakeLists.txt
├── README.md                     ← этот файл
└── src/
    ├── states.h                  # перечисления состояний
    ├── bridge.c / bridge.h       # автомат моста + инварианты
    ├── config.c / config.h       # разбор argv, печать конфигурации
    ├── rng.c / rng.h             # xorshift64*
    ├── time_util.c / time_util.h # CLOCK_MONOTONIC, nanosleep
    ├── stream.c / stream.h       # ring-очереди в SHM (cars/ships)
    ├── shared.h                  # SharedState (всё в SHM)
    ├── ipc.c / ipc.h             # shm_open + sem_open + mutex_lock/unlock
    ├── logger.c / logger.h       # вывод в stdout и sim.log
    ├── signals.c / signals.h     # SIGINT/SIGTERM → shm->stop
    ├── operator.c / operator.h   # логика оператора (автомат, группы судов)
    ├── operator_process.c        # процесс-оператор
    ├── mechanism_process.c       # процесс-механизм моста
    ├── car_generator.c           # процесс-генератор автомобилей
    ├── car_process.c             # процесс одного автомобиля
    ├── ship_generator.c          # процесс-генератор судов (up/down)
    ├── ship_process.c            # процесс одного судна
    ├── processes.h               # точки входа процессов
    └── main.c                    # оркестрация: fork → waitpid → cleanup
```

Сборка:

```bash
cmake -S . -B build
cmake --build build
```

Запуск:

```bash
./build/bridge [опции]
```
