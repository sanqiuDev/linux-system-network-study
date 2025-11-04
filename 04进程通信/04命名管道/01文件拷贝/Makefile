# 伪目标all：默认目标+聚合目标，依赖readfile和readfifo
.PHONY:all
all:readfile readfifo  # 要生成的所有可执行文件都放在这里

# 生成readfile可执行文件的规则
readfile:ReadFile.cc
	g++ -o $@ $^

# 生成readfifo可执行文件的规则
readfifo:ReadFifo.cc
	g++ -o $@ $^

# 定义伪目标clean，用于清理生成的文件
.PHONY:clean
clean:
	rm -f readfile readfifo

	