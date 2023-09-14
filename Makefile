# 注意一定要在书写命令时，必须在命令开头敲一个Tab键，而不能使用4个空格（space）来代替Tab
# 这个模板，以后只需要更改文件名和链接文件夹即可！！!
# 惨痛教训，链接的时候要加上库：@g++ $(OBJS_C) $(OBJS_CPP) -o $(TARGET) -lcurl 

# 根路径
ROOT := $(shell pwd)

# --------------------------修改区----------------------
# 添加链接编译文件的文件夹，有.cpp文件
SUBDIR := $(ROOT)
SUBDIR += $(ROOT)/func
SUBDIR += $(ROOT)/data

# 生成可执行程序的文件名
TARGET := main
# TARGET := test1

# 保存生成文件的文件夹
OUTPUT := ./output

# JSON 头文件路径
JSON_INCLUDE := /home/lb/mylib/json-develop/include/  # 将路径替换为你的 json.hpp 所在位置

# -----------------------------------------------------

# 编译文件夹
INCS := $(foreach dir,$(SUBDIR),-I$(dir))
INCS += -I$(JSON_INCLUDE)  # 添加 JSON 头文件路径

# 筛选出c文件名，链接需要
SRCS_C := $(foreach dir,$(SUBDIR),$(wildcard $(dir)/*.c))
SRCS_CPP := $(foreach dir,$(SUBDIR),$(wildcard $(dir)/*.cpp))
# 筛选出c++文件名，链接需要
OBJS_C := $(patsubst $(ROOT)/%.c,$(OUTPUT)/%.o,$(SRCS_C))
OBJS_CPP := $(patsubst $(ROOT)/%.cpp,$(OUTPUT)/%.o,$(SRCS_CPP))


# g++ 提供了强大的自动生成依赖，记录有依赖关系的 *.d 文件————不用手动为每个源文件添加头文件依赖
DEPS := $(patsubst %.o,%.d,$(OBJS_C))
DEPS += $(patsubst %.o,%.d,$(OBJS_CPP))

# 开始链接
main : $(OBJS_C) $(OBJS_CPP)
	@echo linking...
	@g++ $(OBJS_C) $(OBJS_CPP) -o $(TARGET) -lcurl -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_legacy
	@echo complete!

# c编译
$(OUTPUT)/%.o : %.c
	@echo C compile $<...
	@mkdir -p $(dir $@)
	@gcc -MMD -MP -c $(INCS) $< -o $@ -lsocket -lnetwork -lcurl

# C++编译
$(OUTPUT)/%.o : %.cpp
	@echo C++ compile $<...
	@mkdir -p $(dir $@)
	@g++ -std=c++11 -MMD -MP -c $(INCS) $< -o $@ -lsocket -lnetwork -lcurl -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_legacy

# 删除过程文件
.PHONY : clean

clean:
	@echo try to clean...
	@rm -r $(OUTPUT)
	@echo complete!

-include $(DEPS)
