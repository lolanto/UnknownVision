# TODO LIST

- [ ] 解析shader文件，找到shader中的输入输出。注意vs的输入加上顶点结构的分析，ps的输出加上颜色缓冲，深度缓冲的输出。其次还要考虑如何处理sampler的问题。

- [x] 加载shader文件并缓冲

  - [x] 分析当前shader文件是否已经创建过最新的字节码文件，有则直接读取文件中的字节码

  - [x] 假如当前没有(或者当前已经存在的字节码文件与当前源码不匹配)，则加载源码文件，重新编译并创建新的字节码文件存储

  - [x] 考虑如何缓存shader的字节码

    不缓存shader的字节码，使用的时候从文件系统中读取。

- [ ] PSO应该放在worker thread中创建，但库不应该主动使用多线程技术，应该如何解决这个矛盾

  - [ ] program创建允许多线程同时进行

  - [ ] 同一个shader的blob过程使用锁进行同步，即一个shader名称对应一个锁，假如当前shader需要进行处理则进行

    加锁，待获取完blob后再解锁。从应用层面上进行文件读写的同步(虽然这样很蠢)

  - [ ] Optimization Options:

    - [ ] 允许文件IO同时进行
    - [ ] 对输入的blob进行缓冲，减少io的次数

- [ ] 构建program，分析其总的输入输出，构建DX12需要考虑root signature，descriptor heap