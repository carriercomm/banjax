#ifndef FEATUREVARIANCEREQUESTINTERVAL_H
#define FEATUREVARIANCEREQUESTINTERVAL_H
#include "Feature.h"

class FeatureVarianceRequestInterval:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,float *featureValue);
};
#endif
