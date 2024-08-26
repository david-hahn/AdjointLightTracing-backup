#include "../src/implementations/interactive_adjoint_light_tracing/cmaes/CMAMinimizer.h"
#include "../src/implementations/interactive_adjoint_light_tracing/LBFGSppWrapper.hpp"

extern "C"
{
#include <../src/implementations/interactive_adjoint_light_tracing/cmaes/cmaes.h>
#include <../src/implementations/interactive_adjoint_light_tracing/cmaes/cmaes_interface.h>

	inline double linearlyInterpolate(double v1, double v2, double t1, double t2, double t){
		if (v1 == v2)
			return v2;
		return (t-t1)/(t2-t1) * v2 + (t2-t)/(t2-t1) * v1;
	}
}

CMAMinimizer::CMAMinimizer(int maxIterations, int populationSize, double initialStdDev, double solveFunctionValue, double solveHistToleranceValue)
	:	maxIterations(maxIterations),
		populationSize(populationSize),
		initialStdDev(initialStdDev),
		solveFunctionValue(solveFunctionValue),
		solveHistToleranceValue(solveHistToleranceValue),
		printLevel(0){
}

CMAMinimizer::~CMAMinimizer(){
}

void CMAMinimizer::setBounds(const dVector &pMin, const dVector &pMax){
	this->pMin = pMin;
	this->pMax = pMax;
}

bool CMAMinimizer::minimize(CMAWrapper *function, dVector &p, const dVector& startStdDev, double stdDevModifier, double &functionValue){
	dVector pScaled(p.size());
	bool haveBounds = false;
	
	if (pMin.size() != p.size() || pMax.size() != p.size()) {
	
	
	
		pScaled = p;
	}else{
		
		
		pScaled.setZero();
		for (int i=0; i<p.size(); i++){
			
			
			

			pScaled[i] = (p[i] - pMin[i]) / (pMax[i] - pMin[i]);
		}
		haveBounds = true;
	}
	
	double initialFunctionValue = function->evaluate(p);

	if (printLevel >= 1) {
		
	}

	
	dVector stdDev(startStdDev);
	stdDev *= stdDevModifier;

	
	cmaes_t evo;
	double *arFunVals = cmaes_init(&evo, (int)p.size(), &pScaled[0], &stdDev[0], 0, populationSize, "non");

	evo.sp.stopMaxFunEvals = 1e299;
	evo.sp.stStopFitness.flg = 1;
	evo.sp.stStopFitness.val = solveFunctionValue;
	evo.sp.stopMaxIter = maxIterations;
	evo.sp.stopTolFun = 1e-9;
	evo.sp.stopTolFunHist = solveHistToleranceValue;
	evo.sp.stopTolX = 1e-11;
	evo.sp.stopTolUpXFactor = 1e3;
	evo.sp.seed = 0;

	for (int i = 0;i < pScaled.size();i++)
		evo.rgxbestever[i] = pScaled[i];
	evo.rgxbestever[p.size()] = initialFunctionValue;
	evo.rgxbestever[p.size()+1] = 1;

    assert(cmaes_Get(&evo, "fbestever") == initialFunctionValue);

	
	dVector pi(p.size());
	pi.setZero();

	int iter = 0;
	for (; !cmaes_TestForTermination(&evo); iter++)
	{
		
		double *const *pop = cmaes_SamplePopulation(&evo);
		int popSize = (int)cmaes_Get(&evo, "popsize");
		
		if (printLevel >= 2) {
			
		}

		for (int popIdx=0; popIdx<popSize; popIdx++){
			for (int i=0; i<p.size(); i++)
				if(haveBounds)
					pi[i] = (1 - pop[popIdx][i])*pMin[i] + (pop[popIdx][i])*pMax[i];
				else
					pi[i] = pop[popIdx][i];
			
			
			arFunVals[popIdx] = function->evaluate(pi);
		}

		
		cmaes_UpdateDistribution(&evo, arFunVals);

		
		if (printLevel >= 2)
		{
			dVector pTmp((int)p.size(),0);

			cmaes_GetInto(&evo, "xmean", &pTmp[0]);
			for (int i=0; i<p.size(); i++)
				if(haveBounds)
					pi[i] = (1 - pTmp[i])*pMin[i] + (pTmp[i])*pMax[i];
				else
					pi[i] = pTmp[i];
			double mean = function->evaluate(pi);

			cmaes_GetInto(&evo, "xbest", &pTmp[0]);
			for (int i=0; i<p.size(); i++)
				if(haveBounds)
					pi[i] = (1 - pTmp[i])*pMin[i] + (pTmp[i])*pMax[i];
				else
					pi[i] = pTmp[i];

			double best = function->evaluate(pi);

			cmaes_GetInto(&evo, "xbestever", &pTmp[0]);
			for (int i=0; i<p.size(); i++)
				if(haveBounds)
					pi[i] = (1 - pTmp[i])*pMin[i] + (pTmp[i])*pMax[i];
				else
					pi[i] = pTmp[i];

			double bestEver = function->evaluate(pi);

			
			
			
		}
	}

	
	cmaes_GetInto(&evo, "xbestever", &pScaled[0]);
	for (int i=0; i<p.size(); i++)
		if(haveBounds)
			p[i] = linearlyInterpolate(pMin[i], pMax[i], 0, 1, pScaled[i]);
		else
			p[i] = pScaled[i];
	functionValue = function->evaluate(p);

	if (printLevel >= 1) {
		
		
		

		

	}

	cmaes_exit(&evo);
	return iter < maxIterations;
}


bool CMAMinimizer::minimize(CMAWrapper *function, dVector &p, double &functionValue){
	if (printLevel >= 1) {
		
		
	}
	
	dVector stdDev((int)p.size());
	stdDev.setConstant(initialStdDev);

	return minimize(function, p, stdDev, 1, functionValue);
}
