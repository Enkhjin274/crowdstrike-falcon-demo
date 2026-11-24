/* demo_fixed.c — CrowdStrike Channel File 291 ослын user-space дуурайлга (ЗАССАН хувилбар)
 * Compile: gcc -g -fsanitize=address demo_fixed.c -o demo_fixed */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEMPLATE_FIELDS 21  /* Template type 21 оролтын талбар ТОДОРХОЙЛСОН */
#define SENSOR_FIELDS   20  /* Sensor-ийн код interpreter-т ердөө 20 талбар ӨГДӨГ */
#define MAX_RULES       32  /* Нэг файлаас унших дүрмийн дээд тоо */

/* Нэг дүрэм: "fields[field] == value эсэхийг шалга" */
typedef struct {
    int id;        /* дүрмийн дугаар */
    int field;     /* аль талбарыг унших вэ (index) */
    int value;     /* ямар утгатай харьцуулах вэ */
    int wildcard;  /* 1 бол "*" — ямар ч утга таарна, талбарыг уншихгүй */
} Rule;

/* Channel file-ийг мөр мөрөөр уншиж, дүрмүүдийг rules[] руу хийнэ */
static int load_rules(const char *path, Rule *rules) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(1); }
    char line[128], val[16];
    int n = 0;
    while (n < MAX_RULES && fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;          /* тайлбар, хоосон мөрийг алгасна */
        if (sscanf(line, "%d %d %15s", &rules[n].id, &rules[n].field, val) != 3) continue;
        rules[n].wildcard = (strcmp(val, "*") == 0);               /* "*" бол wildcard */
        rules[n].value    = rules[n].wildcard ? 0 : atoi(val);     /* үгүй бол тоо болгоно */
        n++;
    }
    fclose(f);
    return n;
}

/* Validator-ийг санаатайгаар ӨӨРЧЛӨӨГҮЙ: алдаатай файл одоо ч нэвтэрнэ.
 * Гэхдээ interpreter өөрөө хамгаалалттай тул систем унахгүй (defense in depth). */
static int validate(const Rule *r, int n) {
    for (int i = 0; i < n; i++)
        if (r[i].field < 0 || r[i].field >= TEMPLATE_FIELDS)
            return 0;
    return 1;
}

/* ЗАСВАР: interpreter массивын жинхэнэ уртыг (count) параметрээр авна */
static void interpret(const Rule *r, int n, const int *fields, int count) {
    for (int i = 0; i < n; i++) {
        if (r[i].field < 0 || r[i].field >= count) {               /* <-- FIX: wildcard-аас ч ӨМНӨ bounds check */
            fprintf(stderr, "[interp] WARNING: rule %d wants fields[%d], "
                    "but only %d fields exist -> rule skipped\n",
                    r[i].id, r[i].field, count);
            continue;                                              /* алдаатай дүрмийг алгасаад үргэлжилнэ */
        }
        if (r[i].wildcard) {                                       /* wildcard: уншихгүйгээр MATCH */
            printf("[interp] rule %d: fields[%d] = *  -> MATCH (wildcard, not read)\n",
                   r[i].id, r[i].field);
            continue;
        }
        int actual = fields[r[i].field];                           /* одоо заавал хүрээн дотор */
        printf("[interp] rule %d: fields[%d]=%d vs %d -> %s\n",
               r[i].id, r[i].field, actual, r[i].value,
               actual == r[i].value ? "MATCH" : "no match");
    }
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);                              /* stdout, stderr-ийн дарааллыг хадгална */
    const char *path = argc > 1 ? argv[1] : "channel_file.txt";
    Rule rules[MAX_RULES];
    int n = load_rules(path, rules);

    if (!validate(rules, n)) {
        printf("[validator] REJECTED: %s\n", path);
        return 1;
    }
    printf("[validator] OK: %d rules accepted (limit = %d fields)\n", n, TEMPLATE_FIELDS);

    int *fields = malloc(SENSOR_FIELDS * sizeof *fields);          /* heap дээр 20 int */
    if (!fields) return 1;
    for (int i = 0; i < SENSOR_FIELDS; i++)
        fields[i] = i * 10;                                        /* хуурамч "telemetry" өгөгдөл */

    interpret(rules, n, fields, SENSOR_FIELDS);                    /* уртыг хамт дамжуулна */

    free(fields);
    printf("[sensor] done — still running\n");                     /* процесс хэвийн дуусна */
    return 0;
}
