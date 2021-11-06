# What's KLVM

* KLVM means **K**oa**L**a **V**irtual **M**achine
* KLVM is an intermediate representation and interpreter.
* It was originally designed and developed by ZhuGuangxiang(James) in 2021.

## LSRA

LSRA means **L**inear **S**can **R**egister **A**llocation
[Linear Scan]: https://en.wikipedia.org/wiki/Register_allocation#Linear_scan
"Linear Scan Register Allocation" 1999 Poletto and Sarkar
"Linear Scan Register Allocation for the Java HotSpot™ Client Compiler"
https://hassamuddin.com/blog/reg-alloc/

* Target
  * reduce temporary variables, Koala VM is register-based, like Lua.
  * choose Linear scan register allocation
  * choose None SSA LSRA
  * Spilling Free
* LSRA
  * None SSA
    * need data flow analysis(liveness analysis)
    * iterate instructions vs iterate basic block?
    * global register allocation approach
    * the code is not turned into a graph, all instructions in a linear order
  * SSA
    * no need data flow analysis
    * first def/last use

## SSA

IR SSA form is for optimization, not for register allocation. Is it right?

## OPT
