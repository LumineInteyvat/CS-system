#!/bin/bash

# 定义测试顺序：先测基础用例，最后测 string 和 hello-str
# 排除掉 string 和 hello-str，获取其他所有用例名
OTHERS=$(ls tests/*.c | sed 's/tests\///g' | sed 's/\.c//g' | grep -vE "string|hello-str")

# 拼接最终的测试序列
TEST_CASES="$OTHERS string hello-str"

for test in $TEST_CASES; do
    echo -e "\033[1;33m[Testing $test...]\033[0m"
    
    # 执行测试，-s 保持静默模式，只关注结果
    make ALL=$test run
    
    # 检查上一条指令的退出状态码
    if [ $? -ne 0 ]; then
        echo -e "\033[1;31m[Test $test FAILED! Stop here.]\033[0m"
        exit 1
    fi
    echo -e "\033[1;32m[Test $test PASSED!]\033[0m"
done

echo -e "\033[1;36m[All tests completed successfully!]\033[0m"