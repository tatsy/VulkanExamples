#!/bin/sh

MoltenShaderConverter -gi vert.vert -so vert.spv
MoltenShaderConverter -gi frag.frag -so frag.spv
MoltenShaderConverter -gi smvert.vert -so smvert.spv
MoltenShaderConverter -gi smfrag.frag -so smfrag.spv
