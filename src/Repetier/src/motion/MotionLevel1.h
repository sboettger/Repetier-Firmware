/*
    This file is part of Repetier-Firmware.

    Repetier-Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Repetier-Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Repetier-Firmware.  If not, see <http://www.gnu.org/licenses/>.

*/

/** 
 This is the first level of moves. All moves inserted will start here and be split into
 more detailed data at later stages.
*/
#ifndef _MOTION_LEVEL_1
#define _MOTION_LEVEL_1

extern uint8_t axisBits[NUM_AXES];
extern uint8_t allAxes;
#define SPEED_EPSILON 0.5
#define POSITION_EPSILON 0.001
class Motion2Buffer;
#define FOR_ALL_AXES(i) for (fast8_t i = 0; i < NUM_AXES; i++)
#define MEMORY_POS_SIZE 2

#define FLAG_CHECK_ENDSTOPS 1
#define FLAG_BLOCKED 2

enum Motion1State {
    FREE = 0,              // Not used currenty
    RESERVED = 1,          // Start to fill
    JUNCTION_COMPUTED = 2, // Max. junction speed is computed
    BACKWARD_PLANNED = 3,  // Backward planner run (can run severl times)
    BACKWARD_FINISHED = 4, // Limit speed reached, no improvement possible
    PREPARING = 5,         // Blocked for low level functins, no backplanning here
    FORWARD_PLANNED = 6,   // Forward lanning finished
    RUNNUNG = 7            // Currently being executed
};

enum Motion1Action {
    WAIT = 0,         // Add a wait - in the hope a next move follows for bette roptimization
    LASER_WARMUP = 1, // Preheat laser
    MOVE = 2,         // A plain move
    MOVE_STEPS = 3    // Distance in motor steps given
};

enum EndstopMode {
    DISABLED = 0,        // Endstop tests disabled
    STOP_AT_ANY_HIT = 1, // Stop move on any hit endstop
    STOP_HIT_AXES = 2,   // Continue until all endstops are hit
    PROBING = 3          // Z probing
};

// 158 byte for 7 axes
class Motion1Buffer {
public:
    fast8_t id;
    fast8_t flags;
    Motion1State state;
    Motion1Action action;
    fast8_t axisUsed;
    fast8_t axisDir; // bit set = positive direction
    float start[NUM_AXES];
    float speed[NUM_AXES];
    float unitDir[NUM_AXES];
    float feedrate;
    float acceleration;
    float sa2; // s * a * 2 = v1² - v0²
    secondspeed_t secondSpeed;
    float maxJoinSpeed;
    float startSpeed;
    float endSpeed;
    float length;
    float invLength;

    void computeMaxJunctionSpeed();
    inline bool isAxisMoving(fast8_t axis) {
        return (axisUsed & axisBits[axis]) != 0;
    }
    /* inline bool isDirPositive(fast8_t axis) {
        return (directions & axisBits[axis]) != 0;
    }*/
    float calculateSaveStartEndSpeed();
    void calculateMaxJoinSpeed(Motion1Buffer& prev);
    // Try blocking buffer. Return true on success.
    bool block();
    void unblock();
    inline bool isBlocked() {
        return flags & FLAG_BLOCKED || state == Motion1State::FREE;
    }
    inline bool isCheckEndstops() {
        return flags & FLAG_CHECK_ENDSTOPS;
    }
};
#define EPR_M1_RESOLUTION 0
#define EPR_M1_MAX_FEEDRATE 4 * (NUM_AXES - 1)
#define EPR_M1_MAX_ACCELERATION EPR_M1_MAX_FEEDRATE + 4 * (NUM_AXES - 1)
#define EPR_M1_HOMING_FEEDRATE EPR_M1_MAX_ACCELERATION + 4 * (NUM_AXES - 1)
#define EPR_M1_MAX_YANK EPR_M1_HOMING_FEEDRATE + 4 * (NUM_AXES - 1)
#define EPR_M1_MIN_POS EPR_M1_MAX_YANK + 4 * (NUM_AXES - 1)
#define EPR_M1_MAX_POS EPR_M1_MIN_POS + 4 * (NUM_AXES - 1)
#define EPR_M1_ENDSTOP_DISTANCE EPR_M1_MAX_POS + 4 * (NUM_AXES - 1)
#define EPR_M1_ALWAYS_CHECK_ENDSTOPS EPR_M1_ENDSTOP_DISTANCE + 4 * (NUM_AXES - 1)
#define EPR_M1_AUTOLEVEL_MATRXI +1
#ifdef FEATURE_AXISCOMP
#define EPR_M1_AXIS_COMP_XY EPR_M1_AUTOLEVEL_MATRXI + 36
#define EPR_M1_AXIS_COMP_XZ EPR_M1_AXIS_COMP_XY + 4
#define EPR_M1_AXIS_COMP_YZ EPR_M1_AXIS_COMP_XZ + 4
#define EPR_M1_AXIS_COMP_END EPR_M1_AXIS_COMP_YZ + 4
#else
#define EPR_M1_AXIS_COMP_END EPR_M1_AUTOLEVEL_MATRXI + 36
#endif
#define EPR_M1_TOTAL EPR_M1_AXIS_COMP_END
class Motion1 {
public:
#if EEPROM_MODE != 0
    static uint eprStart;
#endif
    static float autolevelTransformation[9]; ///< Transformation matrix
    static float currentPosition[NUM_AXES];  // Current printer position
    static float currentPositionTransformed[NUM_AXES];
    static float destinationPositionTransformed[NUM_AXES];
    static float tmpPosition[NUM_AXES];
    static float maxFeedrate[NUM_AXES];
    static float homingFeedrate[NUM_AXES];
    static float maxAcceleration[NUM_AXES];
    static float resolution[NUM_AXES];
    static float minPos[NUM_AXES];
    static float maxPos[NUM_AXES];
    static float g92Offsets[NUM_AXES];
    static float maxYank[NUM_AXES];
    static float homeRetestDistance[NUM_AXES];
    static float homeRetestReduction[NUM_AXES];
    static float homeEndstopDistance[NUM_AXES];
#ifdef FEATURE_AXISCOMP
    static float axisCompTanXY, axisCompTanXZ, axisCompTanYZ;
#endif
    static fast8_t homeDir[NUM_AXES];
    static fast8_t homePriority[NUM_AXES]; // determines homing order, lower number first
    static StepperDriverBase* motors[NUM_AXES];
    static fast8_t axesHomed;
    static float memory[MEMORY_POS_SIZE][NUM_AXES + 1];
    static fast8_t memoryPos;
    static EndstopMode endstopMode;
    static int32_t stepsRemaining[NUM_AXES]; // Steps remaining when testing endstops
    static fast8_t alwaysCheckEndstops;
    static fast8_t axesTriggered;
    static fast8_t motorTriggered;
    static fast8_t stopMask; // stop move if these axes are triggered
    /* Buffer is a bit special in the sense that end keeps
    stored until all low level functions are finished while
    mid motion level can already be on an other segment in
    the line.
    */
    static Motion1Buffer buffers[PRINTLINE_CACHE_SIZE]; // Buffer storage
    static volatile fast8_t last;                       /// last entry used for pop
    static volatile fast8_t first;                      /// first entry for reserver
    static volatile fast8_t process;                    /// being processed
    static volatile fast8_t length;                     /// number of entries
    static volatile fast8_t lengthUnprocessed;          /// Number of unprocessed entries
    // Initializes data structures
    static void init();
    // Copy values from Configuration.h
    static void setFromConfig();
    static void fillPosFromGCode(GCode& code, float pos[NUM_AXES], float fallback);
    static void fillPosFromGCode(GCode& code, float pos[NUM_AXES], float fallback[NUM_AXES]);
    // Move with coordinates in official coordinates (before offset, transform, ...)
    static void moveByOfficial(float coords[NUM_AXES], float feedrate);
    // Move to the printer coordinates (after offset, transform, ...)
    static void moveByPrinter(float coords[NUM_AXES], float feedrate);
    // Move with coordinates in official coordinates (before offset, transform, ...)
    static void moveRelativeByOfficial(float coords[NUM_AXES], float feedrate);
    // Move to the printer coordinates (after offset, transform, ...)
    static void moveRelativeByPrinter(float coords[NUM_AXES], float feedrate);
    static void moveRelativeByStepsRelative(int32_t coords[NUM_AXES]);
    static void updatePositionsFromCurrent();
    static void updatePositionsFromCurrentTransformed();
    // Sets A,B,C coordinates to ignore for easy use.
    static void setIgnoreABC(float coords[NUM_AXES]);
    static void copyCurrentOfficial(float coords[NUM_AXES]);
    static void setTmpPositionXYZ(float x, float y, float z);
    static void setTmpPositionXYZE(float x, float y, float z, float e);
    // Reserve new buffer. Waits if required until a buffer is free.
    static void setMotorForAxis(StepperDriverBase* motor, fast8_t axis);
    static void waitForEndOfMoves();
    static void waitForXFreeMoves(fast8_t, bool allowMoves = false);
    static void pop(); // Only called by Motion2::pop !
    static fast8_t buffersUsed();
    /* If possible does a forward step and returns
    the forwarded buffer. If none is available nullptr
    is returned. Will update process and lengthUnprocessed */
    static Motion1Buffer* forward(Motion2Buffer* m2);
    static void insertWaitIfNeeded();
    static void LaserWarmUp(uint32_t wait);
    static void reportBuffers();
    /// Pushes current position to memory stack. Return true on success.
    static bool pushToMemory();
    /// Pop memorized position to tmpPosition
    static bool popFromMemory();
    static void enableMotors(fast8_t axes);
    static bool isAxisHomed(fast8_t axis);
    static void setAxisHomed(fast8_t axis, bool state);
    static void homeAxes(fast8_t axes);
    static void simpleHome(fast8_t axis);
    static void callBeforeHomingOnSteppers();
    static void callAfterHomingOnSteppers();
    static PGM_P getAxisString(fast8_t axis);
    // Moved outside FEATURE_Z_PROBE to allow auto-level functional test on
    // system without Z-probe
    static void transformToPrinter(float x, float y, float z, float& transX, float& transY, float& transZ);
    static void transformFromPrinter(float x, float y, float z, float& transX, float& transY, float& transZ);
#if FEATURE_AUTOLEVEL || defined(DOXYGEN)
    static void resetTransformationMatrix(bool silent);
    //static void buildTransformationMatrix(float h1,float h2,float h3);
    static void buildTransformationMatrix(Plane& plane);
#endif
    static void updateDerived();
    static void eepromHandle();
    static void eepromReset();

private:
    static void backplan(fast8_t actId);
    static Motion1Buffer& reserve();
    static void queueMove(float feedrate);
};
#endif
