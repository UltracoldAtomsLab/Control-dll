/*
void DLL_EXPORT Digital_PrepareDataToSend(
    int32_t nIntArray,
    int32_t* pIntArray,
    int32_t *nContainer,
    uint8_t *pContainer )
{
	int iIntArray = 0;

	int ByteArrayMaxIdx = *nContainer;
	int &iContainer = *nContainer;
	iContainer = 0;

	int channel;
	int n;
	unsigned long long num;

	// repeat time setting
	pContainer[iContainer++] = SET_PERIOD;// indicating to send repeat period
	parse_4(pIntArray[iIntArray++], iContainer, pContainer);// period time data
	pContainer[iContainer++] = RETURN_IDLE;// ending 0

	// setting for each channel
	pContainer[iContainer++] = SET_CH_WAVE;
	while(iIntArray < nIntArray){
        channel = pIntArray[iIntArray++];
		pContainer[iContainer++] = channel%256;
		if(channel == 255)
			break;
		n = pIntArray[iIntArray++];
		parse_2(n, iContainer, pContainer);

		for(int i=0; i<n; ++i){
            num = pIntArray[iIntArray++];
			parse_4(num, iContainer, pContainer);
		}
	}
	pContainer[iContainer++] = RETURN_IDLE;
    return;
}
*/

/*
void DLL_EXPORT Digital_LV_ReadCtrlPoints_To_ArrayOfFPGACommands(
	int32_t iLoop,
	int32_t *nCtrlPtsOfUpCh,
	double  *pCtrlPtsOfUpCh,
	int32_t *nCtrlPtsOfDownCh,
	double  *pCtrlPtsOfDownCh,
	int32_t *nContainer,
	uint8_t *pContainer)
{
	// the double array is store in [which_channel][what_parameter_type][which_control_point]
	//
	// we shift the time by 0.02ms(1 clock-cycle) to reserve the first cycle as the initializing
	// the channel initialized to be 1 would should have its last change-at time equal to 1.

	const int nChannels = 8; // both Up and Down have 8 channels
	const int nCPTKinds = 8; // 8 ctrl point kinds: change-at, change-in, voltage value, and the threes increment, and additional two reserved values

	int maxNumCtrlPtsUp = 0;
	int maxNumCtrlPtsDown = 0;
	// find the maximum as the third dimention of the array.
	for(int iCh = 0; iCh < nChannels; ++iCh)
		if(maxNumCtrlPtsUp < nCtrlPtsOfUpCh[iCh])
			maxNumCtrlPtsUp = nCtrlPtsOfUpCh[iCh];
	for(int iCh = 0; iCh < nChannels; ++iCh)
		if(maxNumCtrlPtsDown < nCtrlPtsOfDownCh[iCh])
			maxNumCtrlPtsDown = nCtrlPtsOfDownCh[iCh];

	const int iChangeAt = 0;
	const int iChangeTo = 2;
	const int iIncChangeAt = 3;

	int ByteArrayMaxIdx = *nContainer;
	int &iContainer = *nContainer;
	uint8_t InitialVoltage[16];
	iContainer = 0;

	// repeat time set to 0
	pContainer[iContainer++] = SET_PERIOD;// indicating to send repeat period
	parse_4(0, iContainer, pContainer);// period time data
	pContainer[iContainer++] = RETURN_IDLE;// ending 0

	pContainer[iContainer++] = SET_CH_WAVE;// indicating to send waveform
	// parsing the waveform
	std::vector<uint32_t> waveform;
	for(int iCh = 0; iCh < nChannels; ++iCh){ // UP-Channel
		double *pChangeAt = pCtrlPtsOfUpCh + ((iCh)*nCPTKinds+iChangeAt)*maxNumCtrlPtsUp; //[iCh][iChangeAt][]
		double *pChangeTo = pCtrlPtsOfUpCh + ((iCh)*nCPTKinds+iChangeTo)*maxNumCtrlPtsUp; //[iCh][iChangeTo][]
		double *pIncChangeAt = pCtrlPtsOfUpCh + ((iCh)*nCPTKinds+iIncChangeAt)*maxNumCtrlPtsUp; //[iCh][iIncChangeAt][]
		waveform.clear();

		// i = 0 case is the initialization
		InitialVoltage[iCh] = (pChangeTo[0] != 0);

		for(int i = 1; i < nCtrlPtsOfUpCh[iCh]; ++i){
			if((pChangeTo[i-1]==0) xor (pChangeTo[i]==0)){
				//uint32_t temp = (uint32_t)((pChangeAt[i]+pIncChangeAt[i]*(iLoop-1))*50000); // 1s = 50M clock-cycles => 1ms = 50k cycles
				uint32_t temp = (uint32_t)((pChangeAt[i]+pIncChangeAt[i]*iLoop)*20); // sampling rate clock
				waveform.push_back(temp);
			}
		}

		// store to the ByteArray
		pContainer[iContainer++] = (uint32_t)iCh;// channel id
		parse_2(waveform.size(), iContainer, pContainer);// number of edges of the channel
		while(!waveform.empty()){ // clock-cycle
			parse_4(waveform.back(), iContainer, pContainer);
			waveform.pop_back();
		}

		// queue like
		//std::vector<uint32_t>::iterator it;
		//for(it=waveform.begin(); it != waveform.end(); it++)
        //    parse_4(*it, iContainer, pContainer);
	}

	for(int iCh = 0; iCh < nChannels; ++iCh){ // Down-Channel
		double *pChangeAt = pCtrlPtsOfDownCh + ((iCh)*nCPTKinds+iChangeAt)*maxNumCtrlPtsDown; //[iCh][iChangeAt][]
		double *pChangeTo = pCtrlPtsOfDownCh + ((iCh)*nCPTKinds+iChangeTo)*maxNumCtrlPtsDown; //[iCh][iChangeTo][]
		double *pIncChangeAt = pCtrlPtsOfDownCh + ((iCh)*nCPTKinds+iIncChangeAt)*maxNumCtrlPtsDown; //[iCh][iIncChangeAt][]
		waveform.clear();

		// i = 0 case is the initialization
		InitialVoltage[iCh+nChannels] = (pChangeTo[0] != 0);

		for(int i = 1; i < nCtrlPtsOfDownCh[iCh]; ++i){
			if((pChangeTo[i-1]==0) xor (pChangeTo[i]==0)){
				//uint32_t temp = (uint32_t)((pChangeAt[i]+pIncChangeAt[i]*(iLoop-1))*50000); // 1s = 50M clock-cycles => 1ms = 50k cycles
				uint32_t temp = (uint32_t)((pChangeAt[i]+pIncChangeAt[i]*iLoop)*20); // sampling rate clock
				temp = temp+1; // the "+1" is for the reserved 1st cycle in which the initialization takes place.
				waveform.push_back(temp);
			}
		}

		// store to the ByteArray
		pContainer[iContainer++] = (uint32_t)iCh+nChannels;// channel id
		parse_2(waveform.size(), iContainer, pContainer);// number of edges of the channel
		while(!waveform.empty()){ // clock-cycle
			parse_4(waveform.back(), iContainer, pContainer);
			waveform.pop_back();
		}

		// queue like
		//std::vector<uint32_t>::iterator it;
		//for(it=waveform.begin(); it != waveform.end(); it++)
        //    parse_4(*it, iContainer, pContainer);
	}

	pContainer[iContainer++] = 255;// sending channel id 255, indicating to terminate
	pContainer[iContainer++] = RETURN_IDLE;

	pContainer[iContainer++] = SET_CH_INIT;
	for(unsigned int i = 0; i <2*nChannels; ++i){
        pContainer[iContainer++] = i;
        pContainer[iContainer++] = InitialVoltage[i];
	}
	pContainer[iContainer++] = 255;// sending channel id 255, indicating to terminate
	pContainer[iContainer++] = RETURN_IDLE;

	pContainer[iContainer++] = CMD_TO_INIT;
	pContainer[iContainer++] = NC;
	pContainer[iContainer++] = RETURN_IDLE;
}
*/
