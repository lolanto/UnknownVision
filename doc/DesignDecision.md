# 架构说明

---

1. 外界输入的是一系列pass实例(但不保证顺序);
2. 同时也允许外界直接调用device的资源创建接口来创建长期存在的GPU资源(如顶点缓存);
3. 允许外界调用device的资源删除接口，声明某GPU资源外界将不再使用

> 删除接口不能直接就删除资源，资源的删除必须同时满足以下两个条件：
>
> 1. GPU没有在使用该资源(**首要条件**)
> 2. 外界应用不使用该资源(**次要条件**)

## Render Backend

backend相当于是某一具体API的工厂，能够枚举当前硬件下的设备及其性能。选择合适的设备并在此之上创建抽象设备Render Device。

> 在DX12中，backend还管理shader的管理工作。

## Render Device

是具体API的Device抽象，负责管理GPU资源的创建和销毁。同时管理Command的翻译执行工作。

## Render Command

是一个或多个具体API调用的集合，这些集合共同构成某种行为，比如Draw, Dispatch等。这些Render Command是self-contain的。

## Render Pass

数个Command构成的，包含一定渲染算法逻辑在内的指令集合。每**种**都有自己的sortkey用于同时需要处理多个pass时，确保处理顺序的正确性。每种pass可以想像为一个class，其构造过程类似于`XXPass(parameter...)`从而产生一个pass实例。

通常某些pass之间是有协助关系的，这些pass之间有自己的前后承接协议(比如规定的输入输出)。

> pass中包含了共用与独占的GPU资源，比如pipeline state object。

> pass的构造过程接收的参数最终会化成pass的显式输入，而pass还拥有隐性输入(比如G-BUFFER)，pass所有的输出都是隐性的。

> 鉴于同一种pass不同实例间的区别很有可能来自于其显式输入，所以假如不同实例间假如显示输入相同，是否意味着这几个实例可以“融合”成一个(只执行一次)?

## Render Handle

是Device创建GPU资源后返回给外界的句柄，用于Device销毁或索引某个资源。

> pass的参数类型只能是handle或者基本数据类型

## Render Graph

通过分析pass，自动管理临时资源(生命周期)以及设置barrier。

>  Render Graph输入的是passes，而输出中应该带有 创建的用于存储临时资源的heap的指令，以及临时资源的aliasing指令？