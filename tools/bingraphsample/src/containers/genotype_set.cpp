////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2018 Olivier Delaneau, University of Lausanne
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////
#include <containers/genotype_set.h>

genotype_set::genotype_set() {
	n_site = 0;
	n_ind = 0;
}

genotype_set::~genotype_set() {
	for (int i = 0 ; i< vecG.size() ; i ++) delete vecG[i];
	vecG.clear();
	n_site = 0;
	n_ind = 0;
}

void genotype_set::solve() {
	tac.clock();
	for (int i = 0 ; i < vecG.size() ; i ++) vecG[i]->solve();
	vrb.bullet("HAP solving (" + stb.str(tac.rel_time()*1.0/1000, 2) + "s)");
}

void genotype_set::sample() {
	tac.clock();
	for (int i = 0 ; i < vecG.size() ; i ++) vecG[i]->sample();
	vrb.bullet("HAP sampling (" + stb.str(tac.rel_time()*1.0/1000, 2) + "s)");
}
