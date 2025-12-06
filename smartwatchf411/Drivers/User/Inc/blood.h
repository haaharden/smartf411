/**
 * ************************************************************************
 * 
 * @file blood.h
 * @author zxr
 * @brief 
 * 
 * ************************************************************************
 * @copyright Copyright (c) 2024 zxr 
 * ************************************************************************
 */
#ifndef _BLOOD_H
#define _BLOOD_H

#include "main.h"
#include "MAX30102.h"
#include "algorithm.h"
#include "math.h"

extern int   heart;   // 算完的心率
extern float SpO2;    // 算完的血氧
void blood_data_translate(void);
void blood_data_update(void);
void blood_Loop(void);

#endif



