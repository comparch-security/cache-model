An Ideal Cache Model Implemented in C++
-------------------------------

#### Authors:
* [Wei Song](mailto:wsong83@gmail.com) (SKLOIS, Institute of Information Engineering, Chinese Academy of Sciences)

#### Cotributors:
* [Zhenzhen Li](mailto:lizhenzhen1@iie.ac.cn) (SKLOIS, Institute of Information Engineering, Chinese Academy of Sciences)
* [Boya Li](https://liboya-cb.github.io/) (SKLOIS, Institute of Information Engineering, Chinese Academy of Sciences)

## Features

* A pure C++ (std-11) and modular design.
* A hierarchical cache supporting the MSI coherence protocol.
* An ideal cache model:
    - Universal access latency: Accessing a cache block has the same latency disregarding to its location (level, set, way or slice) and status (hit or miss).
    - Ideal hit/miss status: Search algorithms can directly and accurately inquire the status of a cache block (hit or miss) with no time penalty.
    - No TLB noise: Virtual addresses are translated into physical addresses before cache accesses.
* Supporting user defined cache set mapping, tagging, replacement functions.
* Supporting the Intel complex addressing scheme.
* Sophisticated tracing and reporting system.

## References

* Wei Song. 支持一致性缓存的Spike仿真器. CRVA联盟技术研讨会, 中国北京, 2020年7月18日. [[PDF](http://wsong83.github.io/presentation/crvs20200718.pdf), [Video](https://www.bilibili.com/video/BV1d5411h79R)]
* Wei Song and Peng Liu. Dynamically finding minimal eviction sets can be quicker than you think for side-channel attacks against the LLC. In Proc. of the International Symposium on Research in Attacks, Intrusions and Defenses (RAID), Beijing, China, pp. 427–442, September 2019. [[Web](https://www.usenix.org/conference/raid2019/presentation/song)]
* Zhenzhen Li and Wei Song. 升级RISC-V的指令级仿真器Spike的缓存模型. In 中国RISC-V论坛, 中国深圳, 2019年11月13日. [[PDF](http://wsong83.github.io/publication/comparch/crvf2019.pdf)]

## Papers Knowingly Utilized This Cache Model

* Wei Song, Boya Li, Zihan Xue, Zhenzhen Li, Wenhao Wang, and Peng Liu. Randomized last level caches are still vulnerable to cache side channel attacks! But we can fix it. In Proceedings of the IEEE Symposium on Security and Privacy (S\&P), Online, pp. 955–969, May 2021.
* Wei Song, Zihan Xue, Jinchi Han, Zhenzhen Li, and Peng Liu. Randomizing set-associative caches against conﬂict-based cache side-channel attacks. IEEE Transactions on Computers, vol. 73, no. 4, pp. 1019-1033, 2024.
* Wei Song, Da Xie, Zihan Xue, and Peng Liu. A parallel tag cache for hardware managed tagged memory in multicore processors. IEEE Transactions on Computers, vol. 73, no. 11, 2488-2503, 2024.
* Anubhav Bhatla, Hari Rohit Bhavsar, Sayandeep Saha, and Biswabandan Panda. SoK: So, You Think You Know All About Secure Randomized Caches? USENIX Security Symposium, August 2025.

## License

GPL.

## Notice

For the model used in the RAID-2019 paper, refer to the [raid-2019](https://github.com/comparch-security/cache-model/tree/raid-2019) branch.

We have re-designed a new cache model namely [FlexiCAS](https://github.com/comparch-security/FlexiCAS), and all following research works have switched to the new cache model.
