
#include "objectivefunction.hpp"




float ObjectiveFunction::weightedResidualNormAndDerivative(const Eigen::Ref<Eigen::VectorXf>& aVertexWeights, const Eigen::Ref<Eigen::VectorXf>& aVertexAreas, 
		const Eigen::Ref<Eigen::VectorXf>& aTarget, const Eigen::Ref<Eigen::VectorXf>& aX, Eigen::Ref<Eigen::VectorXf> aDx){
	
	float O = 0.5 * ((aX - aTarget).transpose() * aVertexWeights.asDiagonal()) * (aVertexAreas.asDiagonal() * (aX - aTarget));
	aDx = aVertexWeights.asDiagonal() * (aVertexAreas.asDiagonal() * (aX - aTarget));
	return O;
}

float SimpleObjectiveFunction::operator()(Eigen::VectorXf& aX, Eigen::VectorXf& aDx){
	aDx.resize(mTarget.size());
	const float phi = weightedResidualNormAndDerivative(mVertexWeights, mVertexAreas, mTarget, aX, aDx);
	return phi;
}

float MultiChannelObjectiveFunction::operator()(Eigen::VectorXf& aX, Eigen::VectorXf& aDx){
	
	Eigen::MatrixXf xMap = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(aX.data(), mTarget.rows(), mTarget.cols());
	Eigen::MatrixXf cMap = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(mVertexColor.data(), mTarget.rows(), 3); 
	Eigen::MatrixXf dMap; dMap.resize(mTarget.rows(), mTarget.cols());
	float phi = 0.0f;
	for (uint32_t k = 0; k < mTarget.cols(); ++k) {
		Eigen::VectorXf entryData = cMap.col(k%3).asDiagonal() * xMap.col(k);
		phi += mChannelWeights(k) * weightedResidualNormAndDerivative(mVertexWeights, mVertexAreas, mTarget.col(k), entryData, dMap.col(k));
		dMap.col(k) = mChannelWeights(k) * cMap.col(k%3).asDiagonal() * dMap.col(k);
	}
	
	aDx.resize(mTarget.rows() * mTarget.cols());
	Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> dxOut(aDx.data(), mTarget.rows(), mTarget.cols());
	dxOut = dMap;
	return phi;
}


float ConsistentMassMultiChannelObjectiveFunction::operator()(Eigen::VectorXf& aX, Eigen::VectorXf& aDx){
	
	Eigen::MatrixXf xMap = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(aX.data(), mTarget.rows(), mTarget.cols());
	Eigen::MatrixXf cMap = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(mVertexColor.data(), mTarget.rows(), 3); 
	Eigen::MatrixXf dMap; dMap.resize(mTarget.rows(), mTarget.cols());
	float phi = 0.0f;
	for (uint32_t k = 0; k < mTarget.cols(); ++k) {
		Eigen::VectorXf entryData =  xMap.col(k).cwiseProduct( cMap.col(k%3) );
		phi += mChannelWeights(k) * 0.5 * ((entryData - mTarget.col(k)).transpose()) * (mM * (entryData - mTarget.col(k)));
		dMap.col(k) = mChannelWeights(k) * cMap.col(k%3).asDiagonal() * (mM * (entryData - mTarget.col(k)));
	}
	
	aDx.resize(mTarget.rows() * mTarget.cols());
	Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> dxOut(aDx.data(), mTarget.rows(), mTarget.cols());
	dxOut = dMap;
	return phi;
}

void ConsistentMassMultiChannelObjectiveFunction::buildConsistentMassMatrix(Eigen::MatrixXi& elems, Eigen::MatrixXf& coords, const Eigen::Ref<Eigen::VectorXf>& aVertexWeights){
	const unsigned int nCoordsPerNode = coords.cols();
	const unsigned int nNodesPerElem = elems.cols();
	const unsigned int nNodes = coords.rows();
	const unsigned int nElems = elems.rows();

	std::vector< Eigen::Triplet<float> > triplets;
	for(unsigned int k=0; k<nElems; ++k){
		unsigned int adof = elems(k,0);
		unsigned int bdof = elems(k,1);
		unsigned int cdof = elems(k,2);
		Eigen::Vector3f a(coords.row(adof)), b(coords.row(bdof)), c(coords.row(cdof));
		
		float w_avg = (aVertexWeights(adof) + aVertexWeights(bdof) + aVertexWeights(cdof)) /3.0;
		float ar = w_avg * 0.5*( (b-a).cross(c-a) ).norm() ;
		triplets.push_back(Eigen::Triplet<float>( adof , adof , ar/6.0 ));
		triplets.push_back(Eigen::Triplet<float>( bdof , adof , ar/12.0 ));
		triplets.push_back(Eigen::Triplet<float>( cdof , adof , ar/12.0 ));
		triplets.push_back(Eigen::Triplet<float>( adof , bdof , ar/12.0 ));
		triplets.push_back(Eigen::Triplet<float>( bdof , bdof , ar/6.0 ));
		triplets.push_back(Eigen::Triplet<float>( cdof , bdof , ar/12.0 ));
		triplets.push_back(Eigen::Triplet<float>( adof , cdof , ar/12.0 ));
		triplets.push_back(Eigen::Triplet<float>( bdof , cdof , ar/12.0 ));
		triplets.push_back(Eigen::Triplet<float>( cdof , cdof , ar/6.0 ));
	}
	mM.resize(nNodes,nNodes);
	mM.setFromTriplets(triplets.begin(), triplets.end());
}
