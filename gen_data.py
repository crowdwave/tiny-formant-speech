#!/usr/bin/env python3
# Emits the data block (phoneme coefficients + vocabulary) for formant_speech.h.
# Vowel formant frequencies are standard published values (Peterson & Barney, 1952; public domain).
# Usage: python3 gen_data.py   (prints the block; paste into the IMPLEMENTATION section, or use the Makefile)
import math
FS=8000; R=0.975; SLOW=1.6
P = {
 'SIL':([0,0,0],0,0),
 'IY':([270,2290,3010],1,1900),'IH':([400,1900,2550],1,1900),'EH':([530,1840,2480],1,1900),
 'AE':([660,1700,2400],1,1900),'AH':([730,1090,2440],1,1900),'AO':([570,840,2410],1,1900),
 'OW':([450,800,2400],1,1900),'UW':([300,870,2240],1,1900),'ER':([490,1350,1690],1,1900),
 'AY':([600,1700,2500],1,1900),
 'L':([360,1300,2500],1,1700),'R':([490,1350,1690],1,1700),'W':([300,610,2200],1,1600),
 'Y':([270,2200,3000],1,1600),'M':([250,1100,2200],1,1500),'N':([250,1700,2400],1,1500),
 'D':([300,1700,2600],1,1300),'G':([300,1100,2200],1,1300),'V':([350,1100,2400],1,900),
 'Z':([330,1700,2600],1,900),'HH':([500,1500,2400],0,700),
 'F':([400,1100,2400],0,750),'TH':([400,1700,2600],0,700),'S':([400,1700,3000],0,800),
 'T':([400,1700,2600],0,800),'K':([350,1100,2200],0,800),
}
def cf(F):
    o=[]
    for f in F:
        if f<=0: o+=[0,0]; continue
        w=2*math.pi*f/FS; o+=[int(round(2*R*math.cos(w)*16384)), int(round(-R*R*16384))]
    return o
order=list(P.keys())
WORDS=[
 ("HELLO",["HH","EH","L","OW"],[55,115,70,170]),("WORLD",["W","ER","L","D"],[70,130,70,90]),
 ("ZERO",["Z","IH","R","OW"],[80,80,70,170]),("ONE",["W","AH","N"],[70,130,100]),
 ("TWO",["T","UW"],[45,210]),("THREE",["TH","R","IY"],[90,60,190]),
 ("FOUR",["F","AO","R"],[90,150,100]),("FIVE",["F","AY","V"],[90,190,90]),
 ("READY",["R","EH","D","IY"],[80,110,70,160]),("GO",["G","OW"],[60,210]),
 ("SAM",["S","AE","M"],[90,150,120]),("VIDEO",["V","IH","D","IY","OW"],[80,80,60,120,150]),
]
def sim(seq,durs,pitch):
    y1=[0]*3;y2=[0]*3;pc=0;lf=0xACE1;peak=0
    for ph,dur in zip(seq,durs):
        F,v,a=P[ph];c=cf(F);ns=int(dur*SLOW)*FS//1000
        for n in range(ns):
            if v: src=(a if pc<=0 else 0); pc=(pitch if pc<=0 else pc); pc-=1
            else: lf=((lf>>1)^(-(lf&1)&0xB400))&0xFFFF; src=((lf&0xFF)-128)*a>>7
            out=0
            for k in range(3):
                y=((c[2*k]*y1[k]+c[2*k+1]*y2[k])>>14)+src
                y=max(-100000,min(100000,y));y2[k]=y1[k];y1[k]=y;out+=y
            peak=max(peak,abs(out))
    return peak
peak=max(sim(s,d,67) for _,s,d in WORDS); sh=0
while (peak>>sh)>112: sh+=1
seqflat=[];off=[];ln=[]
for _,s,d in WORDS:
    off.append(len(seqflat)); ln.append(len(s))
    for ph,dur in zip(s,d): seqflat+=[order.index(ph),int(dur*SLOW)]
o=[]
o.append('/* ---- generated data (gen_data.py): formant coefficients (Q14) + vocabulary ---- */')
o.append('#define FS_SHIFT %d'%sh)
o.append('typedef struct { int16_t a1[3], a2[3]; int16_t amp; uint8_t voiced; } fs_phon_t;')
o.append('static const fs_phon_t fs_phon_[] = {')
for k in order:
    F,v,a=P[k];c=cf(F)
    o.append('  {{%d,%d,%d},{%d,%d,%d},%d,%d}, /* %s */'%(c[0],c[2],c[4],c[1],c[3],c[5],a,v,k))
o.append('};')
o.append('static const char *const fs_text_[] = {%s};'%','.join('"%s"'%w for w,_,_ in WORDS))
o.append('static const uint8_t  fs_seq_[] = {%s};'%','.join(map(str,seqflat)))
o.append('static const uint16_t fs_off_[] = {%s};'%','.join(map(str,off)))
o.append('static const uint8_t  fs_len_[] = {%s};'%','.join(map(str,ln)))
o.append('#define FS_WORDS_ %d'%len(WORDS))
o.append('static const uint8_t fs_pitch_[] = {67,53};  /* voice variants ~120 Hz, ~150 Hz */')
o.append('#define FS_VOICES_ 2')
print('\n'.join(o))
