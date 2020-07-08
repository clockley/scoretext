#include "ner.h"
#include "textutils.h"
#include <mitie.h>
#include <pthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static  mitie_named_entity_extractor *  model;
static __thread char ** tokens;
static __thread mitie_named_entity_detections* entities;

bool initNer(void) {
    pthread_mutex_lock(&mutex);
    model = mitie_load_named_entity_extractor("/usr/local/lib/ner_model.dat");
    pthread_mutex_unlock(&mutex);
    if (model == NULL) {
        return false;
    }
    return true;
}

bool freeNer(void) {
    pthread_mutex_lock(&mutex);
    mitie_free(model);
    pthread_mutex_unlock(&mutex);
    return true;
}

bool loadText(char* text) {
    pthread_mutex_lock(&mutex);
    tokens = mitie_tokenize(text);
    entities = mitie_extract_entities(model, tokens);
    pthread_mutex_unlock(&mutex);
    return true;
}

bool freeText(void) {
    pthread_mutex_lock(&mutex);
    mitie_free(tokens);
    mitie_free(entities);
    pthread_mutex_unlock(&mutex);
}

static void printEntity(FILE * fp, unsigned long i) {
    var p = mitie_ner_get_detection_position(entities, i);
    var l = mitie_ner_get_detection_length(entities, i);
    while (l > 0) {
        fprintf(fp, "%s\n", tokens[p++]);
        --l;
    }
}

char * getWords(void) {
    static __thread char * ptr = NULL;
    size_t s = 0;
    pthread_mutex_lock(&mutex);
    FILE * fp = open_memstream(&ptr, &s);
    var num = mitie_ner_get_num_detections(entities);
    for (size_t i = 0; i < num; ++num) {
        printEntity(fp, i);
    }
    fclose(fp);
    mitie_free(tokens);
    mitie_free(entities);
    return ptr;
}