#ifndef INPUT_WINDOW_VSIZE_A
#define INPUT_WINDOW_VSIZE_A     DIM_A * DIM_B * NUM_FRAMES
#endif
#ifndef INPUT_WINDOW_VSIZE_B
#define INPUT_WINDOW_VSIZE_B     DIM_B *NUM_FRAMES
#endif

#ifndef INPUT_SAMPLES_A
#define INPUT_SAMPLES_A INPUT_WINDOW_VSIZE_A * NITER
#endif
#ifndef INPUT_SAMPLES_B
#define INPUT_SAMPLES_B INPUT_WINDOW_VSIZE_B * NITER
#endif
#define OUTPUT_SAMPLES INPUT_SAMPLES_B