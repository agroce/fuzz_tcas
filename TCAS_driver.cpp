#include <deepstate/DeepState.hpp>
using namespace deepstate;

/*  -*- Last-Edit:  Fri Jan 29 11:13:27 1993 by Tarak S. Goradia; -*- */
/* $Log: tcas.c,v $
 * Revision 1.2  1993/03/12  19:29:50  foster
 * Correct logic bug which didn't allow output of 2 - hf
 * */

#include <stdio.h>

#define OLEV       600		/* in feets/minute */
#define MAXALTDIFF 600		/* max altitude difference in feet */
#define MINSEP     300          /* min separation in feet */
#define NOZCROSS   100		/* in feet */
				/* variables */

int Cur_Vertical_Sep;
bool High_Confidence;
bool Two_of_Three_Reports_Valid;

int Own_Tracked_Alt;
int Own_Tracked_Alt_Rate;
int Other_Tracked_Alt;

int Alt_Layer_Value;		/* 0, 1, 2, 3 */
int Positive_RA_Alt_Thresh[4];

int Up_Separation;
int Down_Separation;

				/* state variables */
int Other_RAC;			/* NO_INTENT, DO_NOT_CLIMB, DO_NOT_DESCEND */
#define NO_INTENT 0
#define DO_NOT_CLIMB 1
#define DO_NOT_DESCEND 2

int Other_Capability;		/* TCAS_TA, OTHER */
#define TCAS_TA 1
#define OTHER 2

int Climb_Inhibit;		/* true/false */

#define UNRESOLVED 0
#define UPWARD_RA 1
#define DOWNWARD_RA 2

void initialize()
{
    Positive_RA_Alt_Thresh[0] = 400;
    Positive_RA_Alt_Thresh[1] = 500;
    Positive_RA_Alt_Thresh[2] = 640;
    Positive_RA_Alt_Thresh[3] = 740;
}

bool Own_Below_Threat()
{
    return (Own_Tracked_Alt < Other_Tracked_Alt);
}

bool Own_Above_Threat()
{
    return (Other_Tracked_Alt < Own_Tracked_Alt);
}

int ALIM ()
{
 return Positive_RA_Alt_Thresh[Alt_Layer_Value];
}

int Inhibit_Biased_Climb ()
{
    return (Climb_Inhibit ? Up_Separation + NOZCROSS : Up_Separation);
}

bool Non_Crossing_Biased_Climb()
{
    int upward_preferred;
    int upward_crossing_situation;
    bool result;

    upward_preferred = Inhibit_Biased_Climb() > Down_Separation;
    if (upward_preferred)
    {
	result = !(Own_Below_Threat()) || ((Own_Below_Threat()) && (!(Down_Separation >= ALIM())));
    }
    else
    {	
	result = Own_Above_Threat() && (Cur_Vertical_Sep >= MINSEP) && (Up_Separation >= ALIM());
    }
    return result;
}

bool Non_Crossing_Biased_Descend()
{
    int upward_preferred;
    int upward_crossing_situation;
    bool result;

    upward_preferred = Inhibit_Biased_Climb() > Down_Separation;
    if (upward_preferred)
    {
	result = Own_Below_Threat() && (Cur_Vertical_Sep >= MINSEP) && (Down_Separation >= ALIM());
    }
    else
    {
	result = !(Own_Above_Threat()) || ((Own_Above_Threat()) && (Up_Separation >= ALIM()));
    }
    return result;
}

int alt_sep_test()
{
    bool enabled, tcas_equipped, intent_not_known;
    bool need_upward_RA, need_downward_RA;
    int alt_sep;

    enabled = High_Confidence && (Own_Tracked_Alt_Rate <= OLEV) && (Cur_Vertical_Sep > MAXALTDIFF);
    tcas_equipped = Other_Capability == TCAS_TA;
    intent_not_known = Two_of_Three_Reports_Valid && Other_RAC == NO_INTENT;
    
    alt_sep = UNRESOLVED;
    
    if (enabled && ((tcas_equipped && intent_not_known) || !tcas_equipped))
    {
	need_upward_RA = Non_Crossing_Biased_Climb() && Own_Below_Threat();
	need_downward_RA = Non_Crossing_Biased_Descend() && Own_Above_Threat();
	if (need_upward_RA && need_downward_RA)
        /* unreachable: requires Own_Below_Threat and Own_Above_Threat
           to both be true - that requires Own_Tracked_Alt < Other_Tracked_Alt
           and Other_Tracked_Alt < Own_Tracked_Alt, which isn't possible */
	    alt_sep = UNRESOLVED;
	else if (need_upward_RA)
	    alt_sep = UPWARD_RA;
	else if (need_downward_RA)
	    alt_sep = DOWNWARD_RA;
	else
	    alt_sep = UNRESOLVED;
    }
    
    return alt_sep;
}

TEST(TCAS, Basic)
{
    initialize();
    Cur_Vertical_Sep = DeepState_IntInRange(-10000, 10000);
    High_Confidence = DeepState_Bool();
    Two_of_Three_Reports_Valid = DeepState_Bool();
    Own_Tracked_Alt = DeepState_IntInRange(-10000, 10000);
    Own_Tracked_Alt_Rate = DeepState_IntInRange(-10000, 10000);
    Other_Tracked_Alt = DeepState_IntInRange(-10000, 10000);
    Alt_Layer_Value = DeepState_IntInRange(0, 3);
    Up_Separation = DeepState_IntInRange(-10000, 10000);
    Down_Separation = DeepState_IntInRange(-10000, 10000);
    Other_RAC = DeepState_IntInRange(0, 2);
    Other_Capability = DeepState_IntInRange(0, 2);
    Climb_Inhibit = DeepState_Bool();
    fprintf(stdout,
	    "INPUT:\n  Cur_Vertical_Sep %d\n  High_Confidence %d\n  Two_of_Three_Reports_Valid %d\n"
	    "  Own_Tracked_Alt %d\n  Own_Tracked_Alt_Rate %d\n  Other_Tracked_Alt %d\n  Alt_Layer_Value %d\n"
	    "  Up_Separation %d\n  Down_Separation %d\n  Other_Capability %d\n  Climb_Inhibit %d\n",
	    Cur_Vertical_Sep,
	    High_Confidence,
	    Two_of_Three_Reports_Valid,
	    Own_Tracked_Alt,
	    Own_Tracked_Alt_Rate,
	    Other_Tracked_Alt,
	    Alt_Layer_Value,
	    Up_Separation,
	    Down_Separation,
	    Other_Capability,
	    Climb_Inhibit);

    int result = alt_sep_test();

    fprintf(stdout, "result = %d\n", result);

    int Pos_Thresh = Positive_RA_Alt_Thresh[Alt_Layer_Value];
    
    if ((Up_Separation >= Pos_Thresh)
	&& (Down_Separation < Pos_Thresh)) {
      ASSERT (result != DOWNWARD_RA) << "P1a";;
    }
    if ((Up_Separation < Pos_Thresh)
	&& (Down_Separation >= Pos_Thresh)) {
      ASSERT (result != UPWARD_RA) << "P1b";
    }
    if ((Up_Separation < Pos_Thresh)
	&& (Down_Separation < Pos_Thresh)
	&& (Down_Separation < Up_Separation)) {
      ASSERT (result != DOWNWARD_RA) << "P2a";
    }
    if ((Up_Separation < Pos_Thresh)
	&& (Down_Separation < Pos_Thresh)
	&& (Down_Separation > Up_Separation)) {
      ASSERT (result != UPWARD_RA) << "P2b";
    }
    if ((Up_Separation > Pos_Thresh)
	&& (Down_Separation >= Pos_Thresh)
	&& (Own_Tracked_Alt > Other_Tracked_Alt)) {
      ASSERT (result != DOWNWARD_RA) << "P3a";
    }
    if ((Up_Separation >= Pos_Thresh)
	&& (Down_Separation >= Pos_Thresh)
	&& (Own_Tracked_Alt < Other_Tracked_Alt)) {
      ASSERT (result != UPWARD_RA) << "P3b";
    }
    if (Own_Tracked_Alt > Other_Tracked_Alt) {
      ASSERT (result != DOWNWARD_RA) << "P4a";
    }
    if (Own_Tracked_Alt < Other_Tracked_Alt) {
      ASSERT (result != UPWARD_RA) << "P4b";
    }
    if (Down_Separation < Up_Separation) {
      ASSERT (result != DOWNWARD_RA) << "P5a";
    }
    if (Down_Separation > Up_Separation) {
      ASSERT (result != UPWARD_RA) << "P5b";
    }
    
    fprintf(stdout, "%d\n", result);
}
