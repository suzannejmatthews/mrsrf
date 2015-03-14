## MrsRF ##

Welcome to the Google Code page for **MrsRF**! Some quick facts:

  * **MrsRF**  stands for (M)ap (R)educe (S)peeds up (R)obinson (F)oulds. It is pronounced "Missus Are-Eff".

  * **MrsRF** is a scalable, efficient multi-core algorithm that uses [MapReduce](http://labs.google.com/papers/mapreduce.html) to quickly calculate the all-to-all Robinson Foulds (RF) distance between large numbers of trees. For t trees, this is outputted as a t x t matrix.

  * **MrsRF** can run on multiple nodes and multiple cores. It can even be executed sequentially.

  * **MrsRF** currently works on linux distributions. We have tested MrsRF on the CentOS and Ubuntu platforms.

  * **MrsRF** is built on top of the [Phoenix MapReduce](http://mapreduce.stanford.edu/) framework.

If you're interested in learning more about **MrsRF**, please visit our wiki for [documentation](HowToUse.md) and additional resources. Please contact the authors for further questions.

Funding for MrsRF was supported by the National Science Foundation under grants DEB-0629849 and IIS-0713618.

---

### To cite ###
Suzanne J Matthews and Tiffani L Williams. _"MrsRF: An efficient MapReduce algorithm for analyzing large collections of evolutionary trees"_. BMC Bioinformatics 2010, **11**(Suppl 1):S15. [Read the paper](http://www.biomedcentral.com/1471-2105/11/S1/S15).