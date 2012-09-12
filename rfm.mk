FOLD=rfm12b

INCDIR+=$(FOLD)
CSRC += $(wildcard $(FOLD)/*.c) 
CPPSRC += $(wildcard $(FOLD)/*.cpp)