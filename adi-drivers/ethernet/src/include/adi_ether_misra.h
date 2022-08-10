/*******************************************************************************
 *
 * Copyright(c) 2011-2016 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you
 * agree to the terms of the associated Analog Devices License Agreement.
 *
 ******************************************************************************/

/*!
* @file      adi_ether_misra.h
*
* @brief     Define Misra rule which are suppressed
*
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* MISRA RULES Enabled */
#ifdef _MISRA_RULES
    
/* Rule 2.1(Req) : Assembly language shall be encapsulated and isolated */
#pragma diag (suppress: misra_rule_2_1: "Use of inline Assembly is required")
/* Rule 2.2: Source code shall use only c-style comments  */
#pragma diag(suppress:misra_rule_2_2:"source code shall use c-style comments")
/* Rule 5.3(Req) : A typedef shall be a unique identifier */
#pragma diag (suppress: misra_rule_5_3)
/* Rule 5.4(Req) : A tag name shall be a unique identifier */
#pragma diag (suppress: misra_rule_5_4)
/* Rule 8.7(Req): Objects shall be defined at block scope if they are only accessed from within a single function */
#pragma diag (suppress: misra_rule_8_7: "Source access the driver internal structure once and a pointer to that used there after")
/* Rule 14.7(Req): A function shall have a single point of exit at the end of the function.*/
#pragma diag (suppress: misra_rule_14_7: "Allowing several point of exit increases code readabilty and maintainability")
/* Rule 17.4(Req): Array indexing shall be the only allowed form of pointer arithmetic. */
#pragma diag (suppress: misra_rule_17_4: "This source needs to use pointer indexing")
/* Rule 20.6(Req): The macro offsetof, in library <stddef.h>, shall not be used */
#pragma diag (suppress: misra_rule_20_6: "This source require the use of offset")
/* Rule 5.1: Macro names is greater than 31 characters */
#pragma diag(suppress:misra_rule_5_1:"macro name is more than 31 characters")
/* Rule 6.1: Plain char type shall be only used with chars */	
#pragma diag(suppress:misra_rule_6_1:"plain char shall only be used with chars")
/* Rule 8.1: Functions shall have prototype declarations and prototypes shall be visible */
#pragma diag(suppress:misra_rule_8_1:"functions shall have prototype declarations and prototypes shall be visible")
/* Rule 8.3: Functions parameter in declaration and definition must be identical */
#pragma diag(suppress:misra_rule_8_3:"function parameter in declaration and definition must be identical")
/* Rule 8.8: External object shall have only one declaration */
#pragma diag(suppress:misra_rule_8_8:"external object shall have only one declarition")
/* Rule 8.11: Static storage class shall be used in definitions with internal linkage  */
#pragma diag(suppress:misra_rule_8_11:"static storage class shall be used in defintions with internal linkage")
/* Rule 10.5: Bitwise operators shall use appropriate casts */
#pragma diag(suppress:misra_rule_10_5:"Bit wise operators shall use appropriate casts")
/* Rules 10.1: [a-d]: Implicit conversion of an expression */
#pragma diag(suppress:misra_rule_10_1_a:"Implicit conversion of an expression")
#pragma diag(suppress:misra_rule_10_1_b:"Implicit conversion of an expression")
#pragma diag(suppress:misra_rule_10_1_c:"Implicit conversion of an expression")
#pragma diag(suppress:misra_rule_10_1_d:"Implicit conversion of an expression")
#pragma diag(suppress:misra_rule_10_3:"Implicit conversion of an expression")
/* Rules 11.5: cast shall not remove any const or volatile qualification */
#pragma diag(suppress:misra_rule_11_5:"cast shall not remove any const or volatile qualification")
/* Rule 12.7: Bitwise operators shall use appropriate casts */
#pragma diag(suppress:misra_rule_12_7:"bitwise operators shall not be applied to signed operators")
/* Rule 12.2: Order of expression should evaluate to same value irrespective of order */ 
#pragma diag(suppress:misra_rule_12_2:"order of expression shall be same in any order of evaluation")
/* Rule 12.5: Declaration may not appear after executable */ 
#pragma diag(suppress:misra_rule_12_5:"declaration may not appear after executable")
/* Rule 12.8: Bitwise operators shall use appropriate casts */
#pragma diag(suppress:misra_rule_12_8:"Right hand operator of shift operator shall lie between zero and one less than the width of bits")
/* Rule 13.1: Assigment operators shall not be used in expressions that yield boolean */
#pragma diag(suppress:misra_rule_13_1:"Assignment operators shall not be used in expressions that yield a boolean")
/* Rule 14.3: Before preprocessing a null statement shall only occur on a line by itself" */
#pragma diag(suppress:misra_rule_14_3:"Before preprocessing a null statement shall only occur on a line by itself")
/* Rule 14.9: if expression shall be followed by compound statement */
#pragma diag(suppress:misra_rule_14_9:"if expression shall be follwed by compound statement")
/* Rule 14.10: if else constructs shall be terminated with an else clause */
#pragma diag(suppress:misra_rule_14_10:"if else constructs shall be terminated with an else clause")
/* Rule 16.9: a function identifier shall only be used with either preceding or with a parenthesised param list */
#pragma diag(suppress:misra_rule_16_9:"if expression shall be follwed by compound statement")
/* Rule 5.6: (Adv): No identifier in one namespace should have the same spelling */
#pragma diag(suppress:misra_rule_5_6:"No identifer in one name space should have the same spelling")
#pragma diag(suppress:misra_rule_16_4:"paramerts in function and declartion should be exactly same")
/* Rule 5.7: (Adv): No identifier name should be reused with enums */
#pragma diag(suppress:misra_rule_5_7:"No identifer name should be reused ")
/* Rule 19.1: (Adv):#include statements in a file should only preceded by other preprocessor directives or comments */
#pragma diag(suppress:misra_rule_19_1:"#include statements in a file should only preceded by other preprocessor directives or comments")
/* Rule 19.4: C marcos shall expand to only braced initializers */
#pragma diag(suppress:misra_rule_19_4:"c marcos shall expand to only braced initializers")
/* Rule 19.6: #undef shall not be used */
#pragma diag(suppress:misra_rule_19_6:"#undef shall not be used")
#endif /* _MISRA_RULES */

#ifdef __cplusplus
}
#endif

