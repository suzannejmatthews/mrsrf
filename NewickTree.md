### Creating Newick Trees ###
_Newick format_ is a simple way to represent bifurcating or multifurcating phylogenetic tree structures in text form. Consider the following phylogenetic tree consisting of four organisms, labeled `a` through `d`:
```
         *
        / \
       /   \
      *     *
     / \   / \
    a   b c   d
```

According to this tree, organisms `a` and `b` are closely related to each other, and organisms `c` and `d` are closely related to each other. To represent the relationship that "_x_ and _y_ are related to each other", Newick format uses the syntax

```
(x,y)
```

Relationship statements can be made at the _internal nodes_ of the tree, which represents a predicted ancestral organism, which gave rise to the two child organisms. In the above tree diagram, internal nodes are represented by the `*` character. In the above code snippet, the presence of this internal node is denoted by the comma. We can therefore represent the above tree in Newick form as:

```
((a,b),(c,d));
```

Let's try a more complex example, this time with 7 organisms, labeled `a` through `g`:

```
         *
        / \
       /   \
      *     *
     / \   / \
    *   b /   \
   / \   *     *
  a   c / \   / \
       d   e f   g
```

The relationship in the above tree can be represented by the following Newick string:


```
(((a,c),b),((d,e),(f,g)));
```

Using this representation, we can _recursively_ represent even the most complex trees in string format.

#### Unrooted Trees ####
To represent _unrooted_ trees in Newick format, it suffices to arbitrarily pick a root at one of the internal edges, and create a recursive definition from there. For example the tree:
```
a     c     e
 \____|____/
 /         \
b           d
```

can equally be represented as
```
(((a,b),c),(d,e));
```
and
```
((a,b),(c,(d,e)));
```

### Creating the Newick Tree input for MrsRF ###
The input to MrsRF is a text file containing a list of Newick trees, with a tree on a separate line. It is very important that no other information is located in this file. Therefore, trees in `.nex` or other tree file formats must have extraneous information removed in order to satisfy the requirements for acceptable input into MrsRF. Below is an example file consisting of three trees consisting of 6 taxa each:
```
(((A,B),C),(D,(E,F)));
(((A,B),D),(C,(E,F)));
(((A,B),E),(D,(C,F)));
```

### Additional Notes ###
  * All the trees in the input file **must** share a common set of organisms. In other words, every tree must have the same organisms, and of the same amount. MrsRF was created to compare the collection of trees outputted from phylogenetic search heuristics. Thus, the organism set is assumed to be common amongst all the trees.
  * While the above examples are small, MrsRF is designed to handle very large sets of trees. In the past, we have used MrsRF to successfully compute all-to-all RF distance matrices of upwards of 30,000 trees!
  * MrsRF does not currently support multifurcating trees. It will in the next update.