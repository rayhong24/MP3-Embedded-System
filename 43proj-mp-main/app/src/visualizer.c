#include "visualizer.h"

static uint32_t LEVELS[9][8] = {{0,0,0,0,1,1,1,1},
                        {0,0,0,0,1,1,1,0},
                        {0,0,0,0,1,1,0,0},
                        {0,0,0,0,1,0,0,0},
                        {0,0,0,0,0,0,0,0},
                        {0,0,0,1,0,0,0,0},
                        {0,0,1,1,0,0,0,0},
                        {0,1,1,1,0,0,0,0},
                        {1,1,1,1,0,0,0,0}};

static uint32_t COLORS[6] = {1,2,3,6,7,8};

static volatile void *pR5Base;

static uint32_t LED_array[8] = {0,0,0,0,0,0,0,0};

static int current_color = 0;
static int current_color_index = 0;

static atomic_bool* thread_kill;

static pthread_t vis_thread;

// Return the address of the base address of the ATCM memory region for the R5-MCU
volatile void* getR5MmapAddr(void)
{
    // Access /dev/mem to gain access to physical memory (for memory-mapped devices/specialmemory)
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("ERROR: could not open /dev/mem; Did you run with sudo?");
        exit(EXIT_FAILURE);
    }

    // Inside main memory (fd), access the part at offset BTCM_ADDR:
    // (Get points to start of R5 memory after it's memory mapped)
    volatile void* pR5Base = mmap(0, MEM_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BTCM_ADDR+MSG_OFFSET);
    if (pR5Base == MAP_FAILED) {
        perror("ERROR: could not map memory");
        exit(EXIT_FAILURE);
    }
    close(fd);

    return pR5Base;
}

void freeR5MmapAddr(volatile void* pR5Base)
{
    if (munmap((void*) pR5Base, MEM_LENGTH)) {
        perror("R5 munmap failed");
        exit(EXIT_FAILURE);
    }
}

//Used to update LED array in shared R5 memory 
void setLEDArrayMem(volatile void * r5base, uint32_t* vals){
    for (int i=0; i<8; i++){
        MEM_UINT32(((uint32_t*) r5base)+(8*i)) = vals[7-i];
    }
}

//Used in setLEDArray while playing or can be used directly to manually set the LEDs
void mapLEDs(int color_index, uint32_t* LED_map){
    for (int i=0; i<NUM_LEDS; i++){
        if(LED_map[i] == 1){
            LED_array[i] = color_index;
        }
        else{
            LED_array[i] = 0;
        }
    }
}

void emptyLEDArray(){
    for(int i = 0; i<NUM_LEDS; i++){
        LED_array[i] = 0;
    }
}

int getLevel(short val){
    double m = (double) val;
    int out =  ((m-SHRT_MIN)/(SHRT_MAX-SHRT_MIN))*9;
    if (out >= 9) out = 8;
    // printf("val = %d\n level = %d\n", val, out);
    return out;
}

//Main function used during gameplay to set LEDs based on accel position
void Visualizer_setLEDArray(short val){

    int level = getLevel(val);

    mapLEDs(current_color, LEVELS[level]);
    setLEDArrayMem(pR5Base, LED_array);

    return;
}

void * Visualizer_doState(void * args){
    atomic_bool* kill = (atomic_bool *) args;

    current_color_index = 0;
    current_color = COLORS[current_color_index];

    while(!(*kill)){
        sleep(3);
        current_color_index = (current_color_index+1)%NUM_COLORS;
        current_color = COLORS[current_color_index];
    }
    return NULL;
}

void Visualizer_init(){
    pR5Base = getR5MmapAddr();
    thread_kill = malloc(sizeof(atomic_bool));
    *thread_kill = false;
    pthread_create(&vis_thread, NULL, Visualizer_doState, thread_kill);
    Visualizer_setLEDArray(0);
}

void Visualizer_cleanup(){
    Visualizer_setLEDArray(0);
    *thread_kill = true;
    pthread_join(vis_thread, NULL);
    emptyLEDArray();
    setLEDArrayMem(pR5Base, LED_array);
    freeR5MmapAddr(pR5Base);
    free(thread_kill);
}
