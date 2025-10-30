//used to load sound effect to be played upon interaction
#ifndef EFFECT_LOADER_H
#define EFFECT_LOADER_H

typedef enum { EFFECT_SOUND1, EFFECT_SOUND2, EFFECT_SOUND3, EFFECT_SOUND_COLLECTION_SIZE } EffectCollection;

void EffectLoader_requestToBeQued(EffectCollection);
void EffectLoader_init(void);
void EffectLoader_cleanup(void);



#endif
