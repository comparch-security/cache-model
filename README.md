An Ideal C++ Cache Model
-------------------------------

#### Authors:
* [Wei Song](mailto:wsong83@gmail.com) (Institute of Information Engineering, Chinese Academy of Sciences)

#### Cotributors:
* [Zhenzhen Li](mailto:lizhenzhen1@iie.ac.cn) (Institute of Information Engineering, Chinese Academy of Sciences)

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

* Wei Song and Peng Liu. Dynamically finding minimal eviction sets can be quicker than you think for side-channel attacks against the LLC. In Proc. of the International Symposium on Research in Attacks, Intrusions and Defenses (RAID), Beijing, China, pp. 427–442, September 2019. [[Web](https://www.usenix.org/conference/raid2019/presentation/song)]
* Zhenzhen Li and Wei Song. 升级RISC-V的指令级仿真器Spike的缓存模型. In 中国RISC-V论坛, 中国深圳, 2019年11月13日. [[PDF](http://wsong83.github.io/publication/comparch/crvf2019.pdf)]

## License

GPL.

## Notice

For the model used in the RAID-2019 paper, refer to the [raid-2019](https://github.com/comparch-security/cache-model/tree/raid-2019) branch.
