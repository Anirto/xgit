xgit


##### 什么是xgit

---

xgit是本人毕业设计，是一个用原生c++编写的版本控制软件，其主要简易的模仿git的功能，其核心是增量存储，对于每次更新的的文件只存储其增量。

后续可能考虑对增量进一步压缩，想一想感觉没有太大必要



##### 运行环境

---

###### WIN

使用vs2019打开xgit.vcxpro生成即可

###### Liunx

```shell
git clone 
cd xgit
make
```



##### 怎么使用

---

主要命令有四个：

1. init           创建仓库目录
2. status      查看当前状态
3. commit    提交当前版本
4. reset         撤回到上一个版本

其它命令可以参见man.cpp



##### To Do

---

- [x] Liunx跨平台
- [ ] 如何实现二进制文件（增量？）存储
- [ ] 使用配置文件让用户自定义一些配置
- [ ] 版本跳跃
- [ ] ....







