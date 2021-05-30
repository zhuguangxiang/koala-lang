# What's KLIR

- Koala Intermediate Representation
- It was originally designed and developed by Zhu Guangxiang (James) in 2021

## About LSRA

- LSRA means Linear Scan Register Allocation
- Wiki: [Linear Scan](https://en.wikipedia.org/wiki/Register_allocation#Linear_scan)
- "Linear Scan Register Allocation" 1999 Poletto and Sarkar
- "Linear Scan Register Allocation for the Java HotSpot Client Compiler"
- https://hassamuddin.com/blog/reg-alloc/

### Target

- reduce temporary variables, Koala VM is register-based, like Lua.
- choose Linear scan register allocation
- choose None SSA LSRA
- Spilling Free

### LSRA

- None SSA
  - need data flow analysis(liveness analysis)
  - iterate instructions vs iterate basic block?
  - global register allocation approach
  - the code is not turned into a graph, all instructions in a linear order

- SSA
  - no need data flow analysis
  -first def/last use

## SSA Form

SSA form IR is for optimization, not for register allocation. Is it right?

## OPT
