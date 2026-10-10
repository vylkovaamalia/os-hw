#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

void config_defaults(Config *cfg) {
    if (!cfg) return;

    cfg->car_lambda       = 0.5;
    cfg->ship_lambda_up   = 0.1;
    cfg->ship_lambda_down = 0.1;

    cfg->car_queue_cap  = 32;
    cfg->ship_queue_cap = 16;

    cfg->car_cross_time  = 2.0;
    cfg->ship_pass_time  = 3.0;

    cfg->barrier_close_time = 1.0;
    cfg->barrier_open_time  = 1.0;
    cfg->raise_time         = 4.0;
    cfg->lower_time         = 4.0;

    cfg->max_car_wait  = 60.0;
    cfg->max_ship_wait = 60.0;

    cfg->priority_prob_car  = 0.05;
    cfg->priority_prob_ship = 0.10;

    cfg->group_strategy = GROUP_STRATEGY_FIFO;
    cfg->max_group_size = 5;

    cfg->max_served = 50;
    cfg->shift_end  = -1.0;

    cfg->seed          = 1;
    cfg->display_delay = 0.0;
}

const char *group_strategy_name(GroupStrategy s) {
    switch (s) {
    case GROUP_STRATEGY_FIFO:     return "FIFO";
    case GROUP_STRATEGY_PRIORITY: return "PRIORITY";
    default:                      return "?";
    }
}

static void print_usage(const char *prog) {
    printf("Использование: %s [опции]\n", prog);
    printf("\nИнтенсивности потоков:\n");
    printf("  --car-lambda=DOUBLE        интенсивность автомобилей (по умолч. 0.5)\n");
    printf("  --ship-lambda-up=DOUBLE    интенсивность судов сверху (по умолч. 0.1)\n");
    printf("  --ship-lambda-down=DOUBLE  интенсивность судов снизу  (по умолч. 0.1)\n");
    printf("\nВместимости очередей:\n");
    printf("  --car-cap=INT              вместимость очереди машин (по умолч. 32)\n");
    printf("  --ship-cap=INT             вместимость очереди судов (по умолч. 16)\n");
    printf("\nВремена операций:\n");
    printf("  --car-cross=DOUBLE         время проезда машины (по умолч. 2.0)\n");
    printf("  --ship-pass=DOUBLE         время прохода судна (по умолч. 3.0)\n");
    printf("  --barrier-close=DOUBLE     время закрытия шлагбаумов (по умолч. 1.0)\n");
    printf("  --barrier-open=DOUBLE      время открытия шлагбаумов (по умолч. 1.0)\n");
    printf("  --raise=DOUBLE             время подъёма пролёта (по умолч. 4.0)\n");
    printf("  --lower=DOUBLE             время опускания пролёта (по умолч. 4.0)\n");
    printf("\nТаймауты ожидания:\n");
    printf("  --max-car-wait=DOUBLE      (по умолч. 60.0)\n");
    printf("  --max-ship-wait=DOUBLE     (по умолч. 60.0)\n");
    printf("\nПриоритеты:\n");
    printf("  --prio-prob-car=DOUBLE     вероятность приоритета у машины (по умолч. 0.05)\n");
    printf("  --prio-prob-ship=DOUBLE    вероятность приоритета у судна (по умолч. 0.10)\n");
    printf("\nСтратегия групп судов:\n");
    printf("  --group-strategy=NAME      fifo | priority (по умолч. fifo)\n");
    printf("  --max-group=INT            размер группы за один подъём (по умолч. 5)\n");
    printf("\nЗавершение:\n");
    printf("  --max-served=INT           обслужено участников (по умолч. 50)\n");
    printf("  --shift-end=DOUBLE         модельное время окончания смены (по умолч. выкл.)\n");
    printf("\nПрочее:\n");
    printf("  --seed=UINT                seed ГСЧ (по умолч. 1)\n");
    printf("  --delay=DOUBLE             задержка вывода, сек (по умолч. 0)\n");
    printf("  --help                     эта справка\n");
}

static bool parse_double(const char *s, double *out) {
    if (!s || !*s) return false;
    char *end = NULL;
    double v = strtod(s, &end);
    if (!end || *end != '\0') return false;
    *out = v;
    return true;
}

static bool parse_ulong(const char *s, unsigned long *out) {
    if (!s || !*s) return false;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = v;
    return true;
}

static bool parse_int(const char *s, int *out) {
    if (!s || !*s) return false;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (int)v;
    return true;
}


enum {
    OPT_CAR_LAMBDA = 1000,
    OPT_SHIP_LAMBDA_UP,
    OPT_SHIP_LAMBDA_DOWN,
    OPT_CAR_CAP,
    OPT_SHIP_CAP,
    OPT_CAR_CROSS,
    OPT_SHIP_PASS,
    OPT_BARRIER_CLOSE,
    OPT_BARRIER_OPEN,
    OPT_RAISE,
    OPT_LOWER,
    OPT_MAX_CAR_WAIT,
    OPT_MAX_SHIP_WAIT,
    OPT_PRIO_PROB_CAR,
    OPT_PRIO_PROB_SHIP,
    OPT_GROUP_STRATEGY,
    OPT_MAX_GROUP,
    OPT_MAX_SERVED,
    OPT_SHIFT_END,
    OPT_SEED,
    OPT_DELAY,
    OPT_HELP,
};

bool config_parse_args(Config *cfg, int argc, char **argv) {
    if (!cfg) return false;

    static const struct option long_opts[] = {
        { "car-lambda",       required_argument, NULL, OPT_CAR_LAMBDA },
        { "ship-lambda-up",   required_argument, NULL, OPT_SHIP_LAMBDA_UP },
        { "ship-lambda-down", required_argument, NULL, OPT_SHIP_LAMBDA_DOWN },
        { "car-cap",          required_argument, NULL, OPT_CAR_CAP },
        { "ship-cap",         required_argument, NULL, OPT_SHIP_CAP },
        { "car-cross",        required_argument, NULL, OPT_CAR_CROSS },
        { "ship-pass",        required_argument, NULL, OPT_SHIP_PASS },
        { "barrier-close",    required_argument, NULL, OPT_BARRIER_CLOSE },
        { "barrier-open",     required_argument, NULL, OPT_BARRIER_OPEN },
        { "raise",            required_argument, NULL, OPT_RAISE },
        { "lower",            required_argument, NULL, OPT_LOWER },
        { "max-car-wait",     required_argument, NULL, OPT_MAX_CAR_WAIT },
        { "max-ship-wait",    required_argument, NULL, OPT_MAX_SHIP_WAIT },
        { "prio-prob-car",    required_argument, NULL, OPT_PRIO_PROB_CAR },
        { "prio-prob-ship",   required_argument, NULL, OPT_PRIO_PROB_SHIP },
        { "group-strategy",   required_argument, NULL, OPT_GROUP_STRATEGY },
        { "max-group",        required_argument, NULL, OPT_MAX_GROUP },
        { "max-served",       required_argument, NULL, OPT_MAX_SERVED },
        { "shift-end",        required_argument, NULL, OPT_SHIFT_END },
        { "seed",             required_argument, NULL, OPT_SEED },
        { "delay",            required_argument, NULL, OPT_DELAY },
        { "help",             no_argument,       NULL, OPT_HELP },
        { NULL, 0, NULL, 0 }
    };

    optind = 1;
    opterr = 0;

    int opt;
    while ((opt = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
        switch (opt) {

        case OPT_CAR_LAMBDA:
            if (!parse_double(optarg, &cfg->car_lambda)) {
                fprintf(stderr, "Ошибка: --car-lambda требует число\n");
                return false;
            }
            break;

        case OPT_SHIP_LAMBDA_UP:
            if (!parse_double(optarg, &cfg->ship_lambda_up)) {
                fprintf(stderr, "Ошибка: --ship-lambda-up требует число\n");
                return false;
            }
            break;

        case OPT_SHIP_LAMBDA_DOWN:
            if (!parse_double(optarg, &cfg->ship_lambda_down)) {
                fprintf(stderr, "Ошибка: --ship-lambda-down требует число\n");
                return false;
            }
            break;

        case OPT_CAR_CAP: {
            int v;
            if (!parse_int(optarg, &v) || v <= 0) {
                fprintf(stderr, "Ошибка: --car-cap требует положительное целое\n");
                return false;
            }
            cfg->car_queue_cap = (size_t)v;
            break;
        }

        case OPT_SHIP_CAP: {
            int v;
            if (!parse_int(optarg, &v) || v <= 0) {
                fprintf(stderr, "Ошибка: --ship-cap требует положительное целое\n");
                return false;
            }
            cfg->ship_queue_cap = (size_t)v;
            break;
        }

        case OPT_CAR_CROSS:
            if (!parse_double(optarg, &cfg->car_cross_time)) {
                fprintf(stderr, "Ошибка: --car-cross требует число\n");
                return false;
            }
            break;

        case OPT_SHIP_PASS:
            if (!parse_double(optarg, &cfg->ship_pass_time)) {
                fprintf(stderr, "Ошибка: --ship-pass требует число\n");
                return false;
            }
            break;

        case OPT_BARRIER_CLOSE:
            if (!parse_double(optarg, &cfg->barrier_close_time)) {
                fprintf(stderr, "Ошибка: --barrier-close требует число\n");
                return false;
            }
            break;

        case OPT_BARRIER_OPEN:
            if (!parse_double(optarg, &cfg->barrier_open_time)) {
                fprintf(stderr, "Ошибка: --barrier-open требует число\n");
                return false;
            }
            break;

        case OPT_RAISE:
            if (!parse_double(optarg, &cfg->raise_time)) {
                fprintf(stderr, "Ошибка: --raise требует число\n");
                return false;
            }
            break;

        case OPT_LOWER:
            if (!parse_double(optarg, &cfg->lower_time)) {
                fprintf(stderr, "Ошибка: --lower требует число\n");
                return false;
            }
            break;

        case OPT_MAX_CAR_WAIT:
            if (!parse_double(optarg, &cfg->max_car_wait)) {
                fprintf(stderr, "Ошибка: --max-car-wait требует число\n");
                return false;
            }
            break;

        case OPT_MAX_SHIP_WAIT:
            if (!parse_double(optarg, &cfg->max_ship_wait)) {
                fprintf(stderr, "Ошибка: --max-ship-wait требует число\n");
                return false;
            }
            break;

        case OPT_PRIO_PROB_CAR:
            if (!parse_double(optarg, &cfg->priority_prob_car)) {
                fprintf(stderr, "Ошибка: --prio-prob-car требует число\n");
                return false;
            }
            break;

        case OPT_PRIO_PROB_SHIP:
            if (!parse_double(optarg, &cfg->priority_prob_ship)) {
                fprintf(stderr, "Ошибка: --prio-prob-ship требует число\n");
                return false;
            }
            break;

        case OPT_GROUP_STRATEGY:
            if (strcmp(optarg, "fifo") == 0) {
                cfg->group_strategy = GROUP_STRATEGY_FIFO;
            } else if (strcmp(optarg, "priority") == 0) {
                cfg->group_strategy = GROUP_STRATEGY_PRIORITY;
            } else {
                fprintf(stderr,
                        "Ошибка: --group-strategy ожидает fifo или priority\n");
                return false;
            }
            break;

        case OPT_MAX_GROUP: {
            int v;
            if (!parse_int(optarg, &v) || v <= 0) {
                fprintf(stderr, "Ошибка: --max-group требует положительное целое\n");
                return false;
            }
            cfg->max_group_size = (size_t)v;
            break;
        }

        case OPT_MAX_SERVED:
            if (!parse_int(optarg, &cfg->max_served) || cfg->max_served <= 0) {
                fprintf(stderr, "Ошибка: --max-served требует положительное целое\n");
                return false;
            }
            break;

        case OPT_SHIFT_END:
            if (!parse_double(optarg, &cfg->shift_end)) {
                fprintf(stderr, "Ошибка: --shift-end требует число\n");
                return false;
            }
            break;

        case OPT_SEED: {
            unsigned long v;
            if (!parse_ulong(optarg, &v)) {
                fprintf(stderr, "Ошибка: --seed требует целое без знака\n");
                return false;
            }
            cfg->seed = v;
            break;
        }

        case OPT_DELAY:
            if (!parse_double(optarg, &cfg->display_delay)) {
                fprintf(stderr, "Ошибка: --delay требует число\n");
                return false;
            }
            break;

        case OPT_HELP:
            print_usage(argv[0]);
            return false;

        case '?':
        default:
            if (optopt != 0) {
                fprintf(stderr, "Неизвестная опция. Попробуйте --help.\n");
            } else {
                fprintf(stderr, "Неизвестная опция. Попробуйте --help.\n");
            }
            return false;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Ошибка: лишние аргументы: ");
        for (int i = optind; i < argc; ++i) {
            fprintf(stderr, "%s ", argv[i]);
        }
        fprintf(stderr, "\n");
        return false;
    }

    return true;
}


void config_print(const Config *cfg) {
    if (!cfg) return;

    printf("=== Конфигурация ===\n");
    printf("Интенсивности:   car=%.3f, ship_up=%.3f, ship_down=%.3f\n",
           cfg->car_lambda, cfg->ship_lambda_up, cfg->ship_lambda_down);
    printf("Очереди:         cars=%zu, ships=%zu\n",
           cfg->car_queue_cap, cfg->ship_queue_cap);
    printf("Времена:         car_cross=%.2f, ship_pass=%.2f\n",
           cfg->car_cross_time, cfg->ship_pass_time);
    printf("Шлагбаумы:       close=%.2f, open=%.2f\n",
           cfg->barrier_close_time, cfg->barrier_open_time);
    printf("Пролёт:          raise=%.2f, lower=%.2f\n",
           cfg->raise_time, cfg->lower_time);
    printf("Таймауты:        car=%.2f, ship=%.2f\n",
           cfg->max_car_wait, cfg->max_ship_wait);
    printf("Приоритеты:      car=%.3f, ship=%.3f\n",
           cfg->priority_prob_car, cfg->priority_prob_ship);
    printf("Стратегия:       %s, max_group=%zu\n",
           group_strategy_name(cfg->group_strategy),
           cfg->max_group_size);
    printf("Завершение:      max_served=%d, shift_end=%.2f\n",
           cfg->max_served, cfg->shift_end);
    printf("Прочее:          seed=%lu, delay=%.2f\n",
           cfg->seed, cfg->display_delay);
    printf("====================\n");
}
