// crossPointsOfCircles3D.cpp


#include <mex.h>

#include <matrix.h>
#include <math.h>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <time.h>

#include "basicfuncs.h"

using namespace std;

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
static double mpi = 3.14159265358979323846;

#define TRIPLE_NUM_UNIT	(0x800000 / 2)

struct stCrossInfo {
	float	crossPts[3];
	__int32 rev;
	__int64 nIdx;
};

stCrossInfo crossInfo[TRIPLE_NUM_UNIT];
stCrossInfo crossInfo1[TRIPLE_NUM_UNIT];

double *C_, *R, *N_;

int		nn_;
int		Triple[TRIPLE_NUM_UNIT][3];
int		Triple1[TRIPLE_NUM_UNIT][3];

__int64 numTriple_;
__int64 numPair_;

unsigned char* bpair_;
vector<float> thrpt(3);

char	fnametri[256], fnamepts[256];

void intersect_triples(double* C_, double* R, double* N_);
void remove_unnecessaryInfo();
void sort_crosspoints(double* C_, double* R, double* N_);

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	if (nrhs != 3) {
		mexErrMsgIdAndTxt("crossPointsOfCircles3D:Input",
			"Three inputs are required.");
	}

	// centers of circles
	if (!mxIsDouble(prhs[0]) || mxIsComplex(prhs[0]))
	{
		mexErrMsgIdAndTxt("crossPointsOfCircles3D:Input",
			"The matrix of centers of circles must be type double.");
	}
	if (mxGetN(prhs[0]) != 3)
	{
		mexErrMsgIdAndTxt("crossPointsOfCircles3D:Input",
			"The matrix of centers of circles must have three columes.");
	}

	// radii of circles
	if (!mxIsDouble(prhs[1]) || mxIsComplex(prhs[1]))
	{
		mexErrMsgIdAndTxt("crossPointsOfCircles3D:Input",
			"The matrix of radii of circles must be type double.");
	}
	if (mxGetN(prhs[1]) != 1 && mxGetM(prhs[1]) != 1)
	{
		mexErrMsgIdAndTxt("crossPointsOfCircles3D:Input",
			"The matrix of radii of circles must be a vector.");
	}

	// normal vectors of circles
	if (!mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]))
	{
		mexErrMsgIdAndTxt("crossPointsOfCircles3D:Input",
			"The matrix of normal vectors of circles must be type double.");
	}
	if (mxGetN(prhs[2]) != 3)
	{
		mexErrMsgIdAndTxt("crossPointsOfCircles3D:Input",
			"The matrix of normal vectors of circles must have three columes.");
	}

	if ( mxGetM(prhs[0]) != mxGetM(prhs[2]) ||
		(mxGetM(prhs[0]) != mxGetM(prhs[1]) && mxGetM(prhs[0]) != mxGetN(prhs[1])))
	{
		mexErrMsgIdAndTxt("crossPointsOfCircles3D:Input",
			"The lengths of all inputs must be same.");
	}

	C_ = mxGetPr(prhs[0]);
	R  = mxGetPr(prhs[1]);
	N_ = mxGetPr(prhs[2]);
	nn_ = (int)mxGetM(prhs[0]);

	bpair_ = (unsigned char*)malloc(nn_*(nn_ / 8 + 1));

	sprintf(fnametri, "%s", "triples.dat");
	sprintf(fnamepts, "%s", "crosspts.dat");

	time_t t1, t2;

	time(&t1);
	// intersect triples
	intersect_triples(C_, R, N_);
	time(&t2);
	printf("Time calculating intersected points = %d\n", t2 - t1);

	if (numTriple_ < 4) {
		free(bpair_);
		return;
	}

	time(&t1);
	// remove unnecessary info
	remove_unnecessaryInfo();
	time(&t2);
	printf("Time removing unnecessary info = %d\n", t2 - t1);

	time(&t1);
	// sort cross points
	sort_crosspoints(C_, R, N_);
	time(&t2);
	printf("Sort time = %d\n", t2 - t1);

	// associate outputs
	plhs[0] = mxCreateNumericMatrix(nn_ / 8 + 1, nn_, mxUINT8_CLASS, mxREAL);
	plhs[1] = mxCreateString(fnametri);
	plhs[2] = mxCreateString(fnamepts);

	memcpy(mxGetData(plhs[0]), bpair_, nn_*(nn_ / 8 + 1));

	free(bpair_);
}

void intersect_triples(double* C_, double* R, double* N_)
{
	numTriple_ = 0;
	int cnttrip = 0;

	ofstream ftriples;

	ftriples.open(fnametri, ios_base::out | ios_base::binary | ios_base::trunc);
	if (ftriples.bad()) {
		printf("The file %s was not opened.\n", fnametri);
		return;
	}

	int i, k, j1, j2, j3, m;
	double p[3];
	double u1[3], u2[3], u3[3];
	double v1[3], v2[3], v3[3];
	double c1[3], c2[3], c3[3];
	double n1[3], n2[3], n3[3];
	vector<int> j2s;
	bool flag;

	unsigned char* bpair1 = (unsigned char*)malloc(nn_*(nn_ / 8 + 1));
	memset(bpair1, 0, nn_*(nn_ / 8 + 1));
	memset(bpair_, 0, nn_*(nn_ / 8 + 1));

	standardizeVector(N_, nn_);

	for (j1 = 0; j1 < nn_ - 2; j1++)
	{
		getRow(N_, nn_, n1, j1);
		getRow(C_, nn_, c1, j1);

		get_uv(n1, R[j1], u1, v1);

		j2s.clear(); i = 0;
		flag = true;
		for (j2 = j1; ;)
		{
			if (flag) j2++;
			else {
				if (i >= j2s.size()) break;
				j2 = j2s[i]; i++;
			}
			if (j2 >= nn_ - 1) break;

			getRow(C_, nn_, c2, j2);
			getRow(N_, nn_, n2, j2);

			if (flag) {
				if (!isPossibleCrossCircles(c1, R[j1], n1, c2, R[j2], n2))
					continue;
				if (R[j1] <= R[j2]) {
					if (!cross_circles(c1, R[j1], u1, v1, c2, R[j2], n2, p)) continue;
				}
				else {
					get_uv(n2, R[j2], u2, v2);
					if (!cross_circles(c2, R[j2], u2, v2, c1, R[j1], n1, p)) continue;
				}
					
			}

			if (!flag || R[j1] <= R[j2])
				get_uv(n2, R[j2], u2, v2); ///

			int existTrip = 0;
			k = i;
			for (j3 = j2; ;)
			{
				if (flag) j3++;
				else {
					if (k >= j2s.size()) break;
					j3 = j2s[k]; k++;
				}
				if (j3 >= nn_) break;

				getRow(C_, nn_, c3, j3);
				getRow(N_, nn_, n3, j3);

				if (flag) {
					if (!isPossibleCrossCircles(c1, R[j1], n1, c3, R[j3], n3))
						continue;
					if (R[j1] <= R[j3]) {
						if (!cross_circles(c1, R[j1], u1, v1, c3, R[j3], n3, p)) continue;
					}
					else {
						get_uv(n3, R[j3], u3, v3);
						if (!cross_circles(c3, R[j3], u3, v3, c1, R[j1], n1, p)) continue;
					}

					j2s.push_back(j3);
				}

				if (!isPossibleCrossCircles(c2, R[j2], n2, c3, R[j3], n3))
					continue;
				if (R[j2] <= R[j3]) {
					if (!cross_circles(c2, R[j2], u2, v2, c3, R[j3], n3, p)) continue;
				}
				else {
					if (!flag || R[j1] <= R[j3]) get_uv(n3, R[j3], u3, v3);
					if (!cross_circles(c3, R[j3], u3, v3, c2, R[j2], n2, p)) continue;
				}

				if (!intersectionPoint(p, c1, R[j1], n1, c2, R[j2], n2, c3, R[j3], n3))
					continue;

				Triple[cnttrip][0] = j1;
				Triple[cnttrip][1] = j2;
				Triple[cnttrip][2] = j3;

				m = j1*(nn_ / 8 + 1) + j3 / 8;
				if (!(bpair1[m] & (1 << (j3 % 8)))) {
					bpair1[m] |= 1 << (j3 % 8);
				}
				else {
					bpair_[m] |= 1 << (j3 % 8);
				}

				m = j2*(nn_ / 8 + 1) + j3 / 8;
				if (!(bpair1[m] & (1 << (j3 % 8)))) {
					bpair1[m] |= 1 << (j3 % 8);
				}
				else {
					bpair_[m] |= 1 << (j3 % 8);
				}

				existTrip++;

				cnttrip++;
				numTriple_++;
				if (cnttrip >= TRIPLE_NUM_UNIT) {
					printf("numTriple is over! %d-%d-%d\n", j1, j2, j3);
					ftriples.write((char*)&Triple[0][0], sizeof(int) * 3 * cnttrip);
					cnttrip = 0;
				}
			}

			if (existTrip == 1) {
				m = j1*(nn_ / 8 + 1) + j2 / 8;
				if (!(bpair1[m] & (1 << (j2 % 8)))) {
					bpair1[m] |= 1 << (j2 % 8);
				}
				else {
					bpair_[m] |= 1 << (j2 % 8);
				}
			}
			else if (existTrip >= 2) {
				m = j1*(nn_ / 8 + 1) + j2 / 8;
				bpair_[m] |= 1 << (j2 % 8);
			}

			if (flag) flag = false;
		}
	}

	ftriples.write((char*)&Triple[0][0], sizeof(int) * 3 * cnttrip);
	ftriples.close();

	free(bpair1);
	
	printf("Getting triples finished!\nThe number of triples is %I64d\n", numTriple_);
}

bool isnecessarypoint(int j1, int j2, int j3)
{
	int m1 = j1*(nn_ / 8 + 1) + j2 / 8;
	if (!(bpair_[m1] & (1 << (j2 % 8)))) return false;

	int m2 = j1*(nn_ / 8 + 1) + j3 / 8;
	if (!(bpair_[m2] & (1 << (j3 % 8)))) return false;

	int m3 = j2*(nn_ / 8 + 1) + j3 / 8;
	if (!(bpair_[m3] & (1 << (j3 % 8)))) return false;

	return true;
}

void remove_unnecessaryInfo()
{
	if (numTriple_ < 4) return;

	ifstream ftriples;
	ofstream ftriples1;

	char fnametri0[256], fnametri1[256];
	sprintf(fnametri0, "%s", fnametri);
	sprintf(fnametri1, "%sm", fnametri);

	int cnttrip, cnttrip1, cntpair;
	__int64 numTriple_1 = 0;

	int i, k, m;
	int j1, j2, j3;
	unsigned char* bpair1 = (unsigned char*)malloc(nn_*(nn_ / 8 + 1));
	unsigned char* bpair2 = (unsigned char*)malloc(nn_*(nn_ / 8 + 1));

	while (1)
	{
		if (numTriple_ < 4) break;

		// open triple data to read
		ftriples.open(fnametri0, ios_base::in | ios_base::binary);
		if (ftriples.bad()) {
			printf("The file %s was not opened.\n", fnametri0);
			return;
		}

		// open new triple data file to write
		ftriples1.open(fnametri1, ios_base::out | ios_base::binary | ios_base::trunc);
		if (ftriples1.bad()) {
			printf("The file %s was not opened.\n", fnametri1);
			ftriples.close(); 
			return;
		}

		cnttrip1 = 0; numTriple_1 = 0;

		cnttrip = numTriple_ < TRIPLE_NUM_UNIT ? (int)numTriple_ : TRIPLE_NUM_UNIT;

		memset(bpair1, 0, nn_*(nn_ / 8 + 1));
		memset(bpair2, 0, nn_*(nn_ / 8 + 1));

		ftriples.seekg(0);
		for (i = 0; i < numTriple_ / cnttrip + 1; i++)
		{
			int cnttrip_r = cnttrip;
			if (i == numTriple_ / cnttrip) {
				cnttrip_r = numTriple_ % cnttrip;
				if (cnttrip_r == 0) break;
			}

			ftriples.read((char*)Triple, sizeof(int) * 3 * cnttrip_r);
			for (k = 0; k < cnttrip_r; k++)
			{
				j1 = Triple[k][0]; j2 = Triple[k][1]; j3 = Triple[k][2];

				if (!isnecessarypoint(j1, j2, j3)) continue;

				int m1 = j1*(nn_ / 8 + 1) + j2 / 8;
				int m2 = j1*(nn_ / 8 + 1) + j3 / 8;
				int m3 = j2*(nn_ / 8 + 1) + j3 / 8;

				if (!(bpair1[m1] & (1 << (j2 % 8)))) bpair1[m1] |= 1 << (j2 % 8);
				else bpair2[m1] |= 1 << (j2 % 8);

				if (!(bpair1[m2] & (1 << (j3 % 8)))) bpair1[m2] |= 1 << (j3 % 8);
				else bpair2[m2] |= 1 << (j3 % 8);

				if (!(bpair1[m3] & (1 << (j3 % 8)))) bpair1[m3] |= 1 << (j3 % 8);
				else bpair2[m3] |= 1 << (j3 % 8);

				memcpy(Triple1[cnttrip1], Triple[k], sizeof(int) * 3);
				cnttrip1++; numTriple_1++;
				if (cnttrip1 >= TRIPLE_NUM_UNIT) {
					ftriples1.write((char*)Triple1, sizeof(int) * 3 * cnttrip1);
					cnttrip1 = 0;
				}

			}
		}
		ftriples.close();

		ftriples1.write((char*)Triple1, sizeof(int) * 3 * cnttrip1);
		ftriples1.close();

		char strtmp[256];
		strcpy(strtmp, fnametri0); strcpy(fnametri0, fnametri1); strcpy(fnametri1, strtmp);

		unsigned char* bptmp = bpair_;
		bpair_ = bpair2; bpair2 = bptmp;

		if (numTriple_ == numTriple_1) break;

		numTriple_ = numTriple_1;
	}

	free(bpair1);
	free(bpair2);

	numPair_ = 0;
	for (j1 = 0; j1 < nn_ - 1; j1++) {
		for (j2 = j1 + 1; j2 < nn_; j2++)
		{
			m = j1*(nn_ / 8 + 1) + j2 / 8;
			if (!(bpair_[m] & (1 << (j2 % 8)))) continue;

			m = j2*(nn_ / 8 + 1) + j1 / 8;
			bpair_[m] |= 1 << (j1 % 8);

			numPair_++;
		}
	}

	if (fnametri1[strlen(fnametri1) - 1] != 'm') {
		sprintf(fnametri1, "%s", fnametri0);
	}

	remove(fnametri1);

	printf("Removing unnecessary info finished!\nThe number of triples is %I64d\nThe number of pairs is %I64d\n",
		numTriple_, numPair_);
}

int compare_m(const void *arg1, const void *arg2)
{
	if (*(float*)arg1 < *(float*)arg2) return -1;
	if (*(float*)arg1 == *(float*)arg2) {
		if (*((float*)arg1 + 1) < *((float*)arg2 + 1)) return -1;
		if (*((float*)arg1 + 1) == *((float*)arg2 + 1)) {
			if (*((float*)arg1 + 2) < *((float*)arg2 + 2)) return -1;
			if (*((float*)arg1 + 2) == *((float*)arg2 + 2)) return 0;
			return 1;
		}
		return 1;
	}
	return 1;
}
void sort_crosspoints(double* C_, double* R, double* N_)
{
	if (numTriple_ < 4) return;

	int i, k, m;
	int j1, j2, j3;
	double p[3];
	double c1[3], c2[3], c3[3];
	double n1[3], n2[3], n3[3];

	// open triple data to read
	ifstream ftriples;
	ftriples.open(fnametri, ios_base::in | ios_base::binary);
	if (ftriples.bad()) {
		printf("The file %s was not opened.\n", fnametri);
		return;
	}

	int cnttrip = numTriple_ < TRIPLE_NUM_UNIT ? (int)numTriple_ : TRIPLE_NUM_UNIT;
	int cntgroup = (int)((numTriple_ - 1) / cnttrip + 1);

	int cntsub = TRIPLE_NUM_UNIT / cntgroup;
	vector<float> xvalingrp(cntgroup);

	vector<int> tmpcount(cntgroup);
	vector<fstream> fcrosspttmp(cntgroup);
	char crosstmpfmt[256];
	sprintf(crosstmpfmt, "%s", "crosstmp_%d.dat");

	ftriples.seekg(0);
	for (i = 0; i < cntgroup; i++)
	{
		int cnttrip_r = cnttrip;
		if (i == numTriple_ / cnttrip) {
			cnttrip_r = numTriple_ % cnttrip;
			if (cnttrip_r == 0) break;
		}

		char fnametmp[256];
		sprintf(fnametmp, crosstmpfmt, i);
		fcrosspttmp[i].open(fnametmp, ios_base::out | ios_base::in | ios_base::binary | ios_base::trunc);
		if (fcrosspttmp[i].bad()) {
			printf("The file %s was not opened.\n", fnametmp);
			ftriples.close();
			return;
		}

		ftriples.read((char*)Triple, sizeof(int) * 3 * cnttrip_r);
		for (k = 0; k < cnttrip_r; k++)
		{
			j1 = Triple[k][0]; j2 = Triple[k][1]; j3 = Triple[k][2];

			getRow(N_, nn_, n1, j1); getRow(C_, nn_, c1, j1);
			getRow(N_, nn_, n2, j2); getRow(C_, nn_, c2, j2);
			getRow(N_, nn_, n3, j3); getRow(C_, nn_, c3, j3);

			crossInfo[k].nIdx = cnttrip_r*i + k;
			if (intersectionPoint(p, c1, R[j1], n1, c2, R[j2], n2, c3, R[j3], n3)) {
				crossInfo[k].crossPts[0] = (float)p[0];
				crossInfo[k].crossPts[1] = (float)p[1];
				crossInfo[k].crossPts[2] = (float)p[2];
			}
			else {
				crossInfo[k].crossPts[0] = (float)1.0e13;
				crossInfo[k].crossPts[1] = (float)1.0e13;
				crossInfo[k].crossPts[2] = (float)1.0e13;
			}
		}
		
		qsort(crossInfo, cnttrip_r, sizeof(stCrossInfo), compare_m);
		if (cntsub >= cnttrip_r) {
			xvalingrp[i] = crossInfo[cnttrip_r - 1].crossPts[0];
		}
		else {
			for (k = 1; k < cntsub; k++) {
				if (crossInfo[cntsub - k].crossPts[0]
					< crossInfo[cntsub - k + 1].crossPts[0]) break;
			}
			xvalingrp[i] = crossInfo[cntsub - k].crossPts[0];
		}

		fcrosspttmp[i].write((char*)&crossInfo[0], sizeof(stCrossInfo) * cnttrip_r);
		tmpcount[i] = cnttrip_r;
	}
	ftriples.close();

	ofstream fcrosspts;
	sprintf(fnamepts, "%s", "crosspts.dat");
	fcrosspts.open(fnamepts, ios_base::out | ios_base::binary | ios_base::trunc);
	if (fcrosspts.bad()) {
		printf("The file %s was not opened.\n", fnamepts);
		return;
	}

	//ofstream fcrsptIdx;
	//char fnamecrsptIdx[256];
	//sprintf(fnamecrsptIdx, "%s", "crssptIdx.dat");
	//fcrsptIdx.open(fnamecrsptIdx, ios_base::out | ios_base::binary | ios_base::trunc);
	//if (fcrsptIdx.bad()) {
	//	printf("The file %s was not opened.\n", fnamecrsptIdx);
	//	return;
	//}

	vector<int> idx1ingrp(numTriple_ / cnttrip + 1, 0);

	//__int64 crssCnt = 0;
	int endCount = 0;
	while (endCount < fcrosspttmp.size())
	{
		float xval = (float)1.0e13;
		for (i = 0; i < xvalingrp.size(); i++) {
			if (idx1ingrp[i] >= tmpcount[i]) continue;
			if (xvalingrp[i] < xval) xval = xvalingrp[i];
		}
		m = 0;
		for (i = 0; i < fcrosspttmp.size(); i++) {
			if (idx1ingrp[i] >= tmpcount[i]) continue;

			fcrosspttmp[i].seekg(0);
			fcrosspttmp[i].read((char*)crossInfo, sizeof(stCrossInfo)*tmpcount[i]);
			for (k = idx1ingrp[i]; k < tmpcount[i]; k++) {
				if (crossInfo[k].crossPts[0] > xval) break;
			}
			int ksub = k;

			memcpy(&crossInfo1[m], &crossInfo[idx1ingrp[i]],
				sizeof(stCrossInfo)*(ksub - idx1ingrp[i]));
			m += ksub - idx1ingrp[i];
			
			idx1ingrp[i] = ksub;
			if (idx1ingrp[i] >= tmpcount[i]) endCount++;
	
			if (ksub + cntsub >= tmpcount[i]) {
				xvalingrp[i] = crossInfo[tmpcount[i] - 1].crossPts[0];
			}
			else {
				for (k = 1; k < cntsub; k++) {
					if (crossInfo[ksub+cntsub - k].crossPts[0]
						< crossInfo[ksub+cntsub - k + 1].crossPts[0]) break;
				}
				xvalingrp[i] = crossInfo[ksub+cntsub - k].crossPts[0];
			}
		}

		qsort(crossInfo1, m, sizeof(stCrossInfo), compare_m);
		fcrosspts.write((char*)&crossInfo1[0], sizeof(stCrossInfo) * m);

		//for (i = 0; i < m; i++) {
		//	__int64 crssidx = crssCnt + i;
		//	fcrsptIdx.seekp(crossInfo1[i].nIdx * sizeof(__int64));
		//	fcrsptIdx.write((char*)&crssidx, sizeof(__int64));
		//}

		//crssCnt += m;
	}

	fcrosspts.close();
	//fcrsptIdx.close();

	for (i = 0; i < fcrosspttmp.size(); i++) fcrosspttmp[i].close();

	for (i = 0; i < fcrosspttmp.size(); i++) {
		char fnametmp[256];
		sprintf(fnametmp, crosstmpfmt, i);
		remove(fnametmp);
	}

}
