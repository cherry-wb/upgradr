// DT == debug trace
#ifdef DT
#  undef DT
#endif 

#ifdef DTI
#  undef DTI
#endif 

#define DT(x) x;
#define DTI(x) x;INDENT_WATCHER;