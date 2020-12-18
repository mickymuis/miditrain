/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

/* Chrono library used to measure execution time of various functions */
#include <chrono>

typedef std::chrono::high_resolution_clock::time_point TimeVarT;

#define duration(a) std::chrono::duration_cast<std::chrono::milliseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()
/* End Chrono part */

