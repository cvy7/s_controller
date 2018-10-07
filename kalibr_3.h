#ifndef KALIBR_1_H
#define KALIBR_1_H

#define CH0_K 7974UL
#define CH0_S 48

#define CH1_K 7959UL
#define CH1_S 48

#define CH2_K 7994UL
#define CH2_S 52

#define CH3_K 7946UL
#define CH3_S 52
//0-10v (0-20ma)
//29- 0v 29502- 10v
#if 0
#define ICH0_K 18215UL
#define ICH0_S 29

#define ICH1_K 18215UL
#define ICH1_S 29

#define ICH2_K 18215UL
#define ICH2_S 29

#define ICH3_K 18215UL
#define ICH3_S 29
#else
//4-20ma
#define ICH0_K 22769UL
#define ICH0_S 5923

#define ICH1_K 22769UL
#define ICH1_S 5923

#define ICH2_K 22769UL
#define ICH2_S 5923

#define ICH3_K 22769UL
#define ICH3_S 5923
#endif

#endif // KALIBR_1_H
