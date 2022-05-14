/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EDF.cpp
 * Author: insus
 * 
 * Created on May 18, 2021, 9:01 PM
 */

#include <cstddef>

#include "EDF.h"
#include <cmath>




EDF::EDF(double adc, double power) : ADCbits(adc), powerVolts(power) {}

EDF::EDF(string file, double adc = 8, double power = 5.0) : ADCbits(adc), powerVolts(power), annotationIndex(-1), p_code(""), annotations(NULL)  {
    read(file);
}

EDF::EDF(const EDF& orig) {
}

EDF::~EDF() {
    delete [] channels;
    delete annotations;
}

void EDF::read(std::string file) {

    /*************************************************************************/
    /*                       PARSE HEADER                                    */
    /*************************************************************************/
    

    stream.open(file, std::ios::in | std::ios::binary);
     if (!stream.is_open() || stream.fail()) {
        cerr << "EDFFile: File '"  << "' does not exist or cannot be read." << endl;
        return;
     }
    stream.seekg(0, std::ios::beg);
    char rootHeaderArray[257] = {0};
    if (stream.fail() || !stream.get(rootHeaderArray, 257)) {
        cerr << "Error trying to read header from file. Giving up..." << endl;
        return;
    }
    
    
    string rootHeader(rootHeaderArray);
    
    int headerLoc = 0;
    string versionStr    = rootHeader.substr(headerLoc, 8);
    headerLoc += 8;    
    
    string patientStr    = rootHeader.substr(headerLoc, 80);
    headerLoc += 80;    
    
    string recordStr     = rootHeader.substr(headerLoc, 80);
    headerLoc += 80;
    
    string startDateStr  = rootHeader.substr(headerLoc, 8);
    headerLoc += 8;
    
    string startTimeStr  = rootHeader.substr(headerLoc, 8);
    headerLoc += 8;
    
    string recordSizeStr = rootHeader.substr(headerLoc, 8);
    headerLoc += 8;
    
    string reservedStr   = rootHeader.substr(headerLoc, 44);
    headerLoc += 44;
    
    string recordCntStr  = rootHeader.substr(headerLoc, 8);
    headerLoc += 8;
    
    string durationStr   = rootHeader.substr(headerLoc, 8);
    headerLoc += 8;
    
    string signalCntStr  = rootHeader.substr(headerLoc, 4);
    
    if (versionStr.compare("0       ") != 0) {
        cerr << "Magic number is wrong." << endl <<
        "All current versions of EDF specifications must" <<
        " start with '0       ' string. Ignoring..." << endl;
        return ;
    }
    

    parse_patient(patientStr);
    parse_startDate(startDateStr, startTimeStr);
    parse_filetype(reservedStr);
    
   if (this->fileType == FileType::EDFPLUS) {
       parsePlusRecordInfo(recordStr); 
   } else { 
       parseStdRecordInfo(recordStr);
   }
    
   this->dataRecordCount = atoi(recordCntStr.c_str());
   this->dataRecordDuration = atof(durationStr.c_str());
   this->signalCount = atoi(signalCntStr.c_str());
    
   if (!this->parse_SignalHeaders(stream)) {
       cerr << "error parsing Signal Heander " << endl;
       return;
   }
   
   if (!validFileLength(stream, atoi(recordSizeStr.c_str()))) {
       cerr << "error parsing 2" << endl;
   }
   
   if (annotationIndex > -1) {
        parseAnnotations(stream);
   }
   recordingTime = dataRecordCount * dataRecordDuration;
   for (int i = 0; i < signalCount; i++ ) {
       if (strcmp("EDF Annotations", trim(channels[i].label).c_str()) == 0) 
       {
           channels[i].signal = new vector<double>();// just for not be null
           continue; 
       }
       channels[i].signal = parseSignal(stream, i, 0, recordingTime);
   }
   
   stream.close();   

   
    cout << "------------------------PARSE--------------------" << endl;
    cout << "######################  PATIENT INFO ##################" << endl;
    cout << "Code=" << this->p_code << endl;
    cout << "Name=" << this->p_name << endl;
    cout << "Birthdate=" << this->p_birthdate << endl;
    cout << "Gender=" << this->p_gender << endl;
    cout << "Additioanl=" << this->p_additional << endl;
    
    cout << "###################### EX INFO ######################" << endl;
    cout << "Start date=" << this->startDate.day << "-" <<this->startDate.month << "-" << this->startDate.year 
            << " " << this->startDate.hh << ":" << startDate.mm << ":" << startDate.ss << endl; 
    cout << "File type EDF =" << (this->fileType == FileType::EDF) << endl;
    cout << "File type EDF+ =" << (this->fileType == FileType::EDFPLUS) << endl;
    cout << "Continusity =" << (this->continuity == Continuity::CONTINUOUS) << endl;
    cout << "DisContinusity =" << (this->continuity == Continuity::DISCONTINUOUS) << endl;
    if (this->fileType == FileType::EDFPLUS) {
        cout << "Admin code " <<this->rs_adminCode << endl;
        cout << "Technician " <<this->rs_technician << endl;
        cout << "Equipment " <<this->rs_equipment << endl;
    } else {
        cout << "Recording " <<this->rs_recording << endl;
    }
    cout << "Data record count " << this->dataRecordCount << endl;
    cout << "Data record duration " << this->dataRecordDuration << endl;
    cout << "Signal count " << this->signalCount << endl;
    
    cout << "------------ CHANNELS------------" << endl;
    for (int i = 0; i < signalCount; i++) {
        cout << channels[i].label 
                << " " << channels[i].transducer 
                << " " << channels[i].physicalDimension 
                << " " << channels[i].physicalMin 
                << " " << channels[i].physicalMax 
                << " " << channels[i].digitalMin 
                << " " << channels[i].digitalMax 
                << " " << channels[i].prefilter
                << " " << channels[i].bufferOffset
                << " " << channels[i].signalSampleCount
                << " " << channels[i].reserved
                << endl;
    }
    cout << "--------------------------------------------------" << endl;
    
    cout << "Data record size " << dataRecordSize << endl;
    cout << "Anotation index  " << annotationIndex << endl;
    if (annotations != NULL && annotations->size() > 0) {
        cout << "------------------ ANNOTATIONS ------------------ " << endl;
        for (auto i = 0; i < annotations->size(); i++) {
            annotation_type_obj obj = annotations->at(i);
            cout << obj.onset << "  " << obj.duration 
                    << " str len " << obj.annotationStrings.size() << endl; 
        }
    }
 
    for (int i = 0; i < signalCount; i++) {
        cout << "Channel " << channels[i].label << " contains double  " 
                << channels[i].signal->size() << endl;
        
    }
    
    for (int i = 0; i < signalCount; i++) {
        if (strcmp("EDF Annotations",channels[i].label.c_str()) == 0) continue;
        cout << channels[i].label << "  " << 12300 << " is " << channels[i].signal->at(12300) <<endl;// << " " << channels[i].signal->at(12301) << " " << channels[i].signal->at(12302) << endl;
    }
    
}

void EDF::parse_patient(const string &pStr) {
    
    // extract patient info and parse into subparts - 80 characters
    // fields are separated by spaces and fields do not contain spaces
    
    size_t fieldLength = 0;
    size_t fieldStart = 0;
    // code
    if ((fieldLength = pStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        this->p_code = pStr.substr(fieldStart, fieldLength);
        fieldStart += fieldLength + 1;
    }
    
    // gender
    if ((fieldLength = pStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        string t = pStr.substr(fieldStart, fieldLength);
        string gender = pStr.substr(fieldStart, fieldLength);
        if (gender.compare("F") == 0)
            this->p_gender = 0;
        else if (gender.compare("M") == 0)
            this->p_gender = 1;
        else
            this->p_gender = -1;
        fieldStart += fieldLength + 1;
    }
    
    // birthdate
    if ((fieldLength = pStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        this->p_birthdate = pStr.substr(fieldStart, fieldLength);
        fieldStart += fieldLength + 1;
    }
    
    // name
    if ((fieldLength = pStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        this->p_name = pStr.substr(fieldStart, fieldLength);
    }
} 

void EDF::parse_startDate(const string &date, const string &time) {
int day, month, year;
    if (date[2] != '.' || date[5] != '.' ||
        date[0] < '0' || date[0] > '9' ||
        date[1] < '0' || date[1] > '9' ||
        date[3] < '0' || date[3] > '9' ||
        date[4] < '0' || date[4] > '9' ||
        date[6] < '0' || date[6] > '9' ||
        date[7] < '0' || date[7] > '9') {
        // formatting error set to first valid date
        day = month = 1;
        year = 85;
    } else {
        day = atoi(date.substr(0, 2).c_str());
        month = atoi(date.substr(3, 2).c_str());
        year = 2000  + atoi(date.substr(6, 2).c_str());
    }
    
    if (day < 1 || day > 31)
        this->startDate.day = 1;
    else
        this->startDate.day = day;
    
    if (month < 1 || month > 12)
        this->startDate.month = 1;
    else
        this->startDate.month = month;
    
    if (year < 0 /*|| year > 99*/)
        this->startDate.year = 85;
    else
        this->startDate.year = year;
const string MONTHS_STR = "JANFEBMARAPRMAYJUNJLYAUGSEPOCTNOVDEC";
this->startDate.monthName = MONTHS_STR.substr((this->startDate.month - 1) * 3, 3);


if (time[2] != '.' || time[5] != '.'  ||
        time[0] < '0' || time[0] > '9' ||
        time[1] < '0' || time[1] > '9' ||
        time[3] < '0' || time[3] > '9' ||
        time[4] < '0' || time[4] > '9' ||
        time[6] < '0' || time[6] > '9' ||
        time[7] < '0' || time[7] > '9')
    {
        this->startDate.ss = this->startDate.mm = this->startDate.hh = 0;
        cerr << "* RT  Could not parse string: " << time << endl;
    } else {
        startDate.ss = atoi(time.substr(0, 2).c_str());
        startDate.mm  = atoi(time.substr(3, 2).c_str());
        startDate.hh  = atoi(time.substr(6, 2).c_str());
    }

if (startDate.ss > 59) {
        startDate.mm += startDate.ss / 60;
        startDate.ss = startDate.ss % 60;
    }
    
    if (startDate.mm > 59) {
        startDate.hh += startDate.mm / 60;
        startDate.mm = startDate.mm % 60;
    }

}

void EDF::parse_filetype(const string &rStr) {

string fileType = rStr.substr(0, rStr.find(' ', 0));
    if (fileType.compare("EDF+C") == 0) {
        this->continuity = Continuity::CONTINUOUS;
        this->fileType = FileType::EDFPLUS;
    } else if (fileType.compare("EDF+D") == 0) {
        this->continuity = Continuity::DISCONTINUOUS;
        this->fileType = FileType::EDFPLUS;
    } else {
        this->continuity = Continuity::CONTINUOUS;
        this->fileType = FileType::EDF;
    }

}


void EDF::parsePlusRecordInfo(const string &recordStr) {
    size_t fieldStart = 0;
    size_t fieldLength = 0;
    
    // subheader
    if ((fieldLength = recordStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        string recordingStart = recordStr.substr(fieldStart, fieldLength);
        if (recordingStart.compare("Startdate") != 0) 
            cerr << "\"Startdate\" text not found in recording information header. Ignoring..." << endl;
        else
            fieldStart += fieldLength + 1;
    }
    
    // start date
    // harder to parse than other version that should be the same
    if ((fieldLength = recordStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        //header.setDate(recordingInfo.substr(fieldStart, spaceLoc));
        fieldStart += fieldLength + 1;
    }
    
    // admin code
    if ((fieldLength = recordStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        this->rs_adminCode = (recordStr.substr(fieldStart, fieldLength));
        fieldStart += fieldLength + 1;
    }
    
    // technician
    if ((fieldLength = recordStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        this->rs_technician = (recordStr.substr(fieldStart, fieldLength));
        fieldStart += fieldLength + 1;
    }
    
    // equipment
    if ((fieldLength = recordStr.find(' ', fieldStart) - fieldStart) != string::npos)
        this->rs_equipment = (recordStr.substr(fieldStart, fieldLength));
}

void EDF::parseStdRecordInfo(const string &recordStr) {
    size_t fieldStart = 0;
    size_t fieldLength = 0;
    
    if ((fieldLength = recordStr.find(' ', fieldStart) - fieldStart) != string::npos) {
        this->rs_recording = recordStr.substr(fieldStart, fieldLength);
    }
}

bool EDF::parse_SignalHeaders(std::fstream &in) {
    // read signal data characters 257 -> signal count * 256
    int signalHeaderLength = this->signalCount * 256 + 1;
    
    char* signalHeaderArray = new char[signalHeaderLength];
    if (in.fail() || !in.get(signalHeaderArray, signalHeaderLength)) {
        cerr << "Error reading header from file. Giving up..." << endl;
        delete [] signalHeaderArray;
        return false;
    }
   
    // verify that header data characters are in valid range
    if (!charactersValid(signalHeaderArray, signalHeaderLength - 1)) {
        delete [] signalHeaderArray;
        return false;
    }
    
    string signalHeader(signalHeaderArray);
    delete [] signalHeaderArray;
    
    size_t headerLoc = 0;
    int signalCount = this->signalCount;
   
    this->channels = new edf_channel_info[signalCount];
    
    // there can only be one annotation channel. if more than one is found then report an error
    int annotationIndex = -1;
     
    // extract signal labels 16 chars at a time
    //range_loop(sigNum, 0, signalCount, 1) {
    for (int sigNum = 0; sigNum < signalCount; sigNum++) {
        
        edf_channel_info cInfo;
        
        cInfo.label = trim(signalHeader.substr(headerLoc, 16));
        channels[sigNum] = cInfo;
        

        if (cInfo.label.compare("EDF Annotations") == 0) {
            if (annotationIndex != -1) {
                cerr << "More than one annotation signals defined. Only the first is accessible..." << endl;
            } else {             
                if (signalAvailable(sigNum)) {
                    annotationIndex = sigNum;
                }
                if (this->fileType == FileType::EDF)
                    cerr << "Annotations not expected in EDF file. Handling them anyway..." << endl;
            }
        }
        headerLoc += 16;
    }
    
    
    // extract transducer types 80 chars at a time
    for(int sigNum = 0; sigNum < signalCount; sigNum++) {
        if (signalAvailable(sigNum)) {
            channels[sigNum].transducer = trim(signalHeader.substr(headerLoc, 80));
        }
        headerLoc += 80;
    }
    
    // extract physical dimensions 8 chars at a time
    for(int sigNum = 0; sigNum < signalCount; sigNum++) {
        if (signalAvailable(sigNum)) {
            channels[sigNum].physicalDimension = trim(signalHeader.substr(headerLoc, 8));
        }
        headerLoc += 8;
    }
    
    // extract physical minimums 8 chars at a time
    for(int sigNum = 0; sigNum < signalCount; sigNum++) {
        if (signalAvailable(sigNum)) {
            channels[sigNum].physicalMin = atoi(signalHeader.substr(headerLoc, 8).c_str());
        }
        headerLoc += 8;
    }
    
    // extract physical maximums 8 chars at a time
    for(int i = 0; i < signalCount; i++) {
        if (signalAvailable(i)) {
            channels[i].physicalMax = atoi(signalHeader.substr(headerLoc, 8).c_str());
        }
        headerLoc += 8;
    }
    
    // extract physical minimums 8 chars at a time
    for(int i = 0; i < signalCount; i++) {
        if (signalAvailable(i)) {
            channels[i].digitalMin = atoi(signalHeader.substr(headerLoc, 8).c_str());
        }
        headerLoc += 8;
    }
    
    // extract physical maximums 8 chars at a time
    for(int i = 0; i < signalCount; i++) {
        if (signalAvailable(i)) {
            channels[i].digitalMax = atoi(signalHeader.substr(headerLoc, 8).c_str());
        }
        headerLoc += 8;
    }
    
    // extract prefilters 80 chars at a time
    for(int i = 0; i < signalCount; i++) {
        if (signalAvailable(i)) {
            channels[i].prefilter = trim(signalHeader.substr(headerLoc, 80));
        }
        headerLoc += 80;
    }
    
    int dataRecordSize = 0;
    int tempOffset = 0;
    // extract the number of samples in each record 8 chars at a time
    for(int i = 0; i < signalCount; i++) {
        if (!signalAvailable(i)) {
            continue;
        }
        int count = channels[i].signalSampleCount = atoi(signalHeader.substr(headerLoc, 8).c_str());
        //int count = //header->signalSampleCount(sigNum);
        dataRecordSize += count;
        headerLoc += 8;
        
        channels[i].bufferOffset = tempOffset;
        // each data point is 2 bytes
        tempOffset += count * 2;
    }
    this->dataRecordSize = dataRecordSize * 2;
    
    // extract reserved field 32 chars at a time
    for(int i = 0; i < signalCount; i++) {
        if (signalAvailable(i)) {
            channels[i].reserved = trim(signalHeader.substr(headerLoc, 32));
        }
        headerLoc += 32;
    }
    
    return true;
}

bool EDF::validFileLength(std::fstream &in, int recordSize) {
// jump to end of file and make sure all data is present and there is no extra data
    in.seekg(0, std::ios::end);
    long long last = in.tellg();
    long long lengthDiff = last - (dataRecordCount) * (dataRecordSize) - recordSize;
    if (lengthDiff < 0) {
        cerr << "Data segement has excessive information. Signal data will not be accessible..." << endl;
        return false;
    } else if (lengthDiff > 0) {
        cerr << "Data segement has missing information. Signal data will not be accessible..." << endl;
        return false;
    }
    
    return true;
}


bool EDF::signalAvailable(int sigNum) const {
    if (sigNum >= 0 && sigNum < this->signalCount)
        return true;
    else
        return false;
}


bool EDF::parseAnnotations(std::fstream &in) {
 // tal = time-stamped annotations list
    int annSigIdx = annotationIndex;
    if (annSigIdx < 0)
        return false;
    
    annotations = new std::vector<annotation_type_obj>();
    
    // seek to beginning of data records
    in.seekg(signalCount * 256 + 256, std::ios::beg);
    
    char* record = new char[dataRecordSize];
    int talStart = channels[annSigIdx].bufferOffset;//header->bufferOffset(annSigIdx);
    int talEnd = talStart + channels[annSigIdx].signalSampleCount /*header->signalSampleCount(annSigIdx)*/ * 2;
    int talLength = talEnd - talStart;
    char* tal = new char[talLength];
    
    for(int recordNum = 0; recordNum < dataRecordCount; recordNum++) {
        string onset, duration;
        std::vector<string> annotationStrings;
        int talOffset = 0;
        
        if(!in.read(record, dataRecordSize /*header->dataRecordSize()*/)) {
            cerr << "Error reading annotations from file. Giving up..." << endl;
            delete annotations;
            delete [] record;
            delete [] tal;
            return false;
        }
        
        // and extract TAL section
        memcpy(tal, record + talStart, talLength);
        
        while (talOffset < talLength) {
            // find length of onset by checking for 20 or 21
            int onsetLength = 0;
            while (tal[talOffset + onsetLength] != 20 &&
                   tal[talOffset + onsetLength] != 21 &&
                   talOffset + onsetLength < talLength) // check if beyond tal length
                onsetLength++;
            
            if (talOffset + onsetLength < talLength) {
                onset = parseOnset(tal, talOffset, onsetLength);
                
                if (tal[talOffset] == 21) { // duration string expected
                    duration = parseDuration(tal, talOffset, onsetLength);
                    
                } else { // skip duration and extract the annotation
                    string ann = parseAnnotation(tal, talOffset);
                    if (ann.size() > 0) // avoid storing empty objects
                        annotationStrings.push_back(trim(ann));
                }
                
            } else {
                talOffset += onsetLength;
            }
        }
        
        // avoid storing empty objects
        if (annotationStrings.size() > 0) { 
            annotation_type_obj an;
            an.onset = atof(onset.c_str());
            an.duration = atof(duration.c_str());
            an.annotationStrings = annotationStrings;            
            annotations->push_back(an);
        }
    }
    
    delete [] tal;
    delete [] record;
    return true;
}

string EDF::parseOnset(char* const &tal, int &talOffset, int onsetLength) {
    // verify onset character set
    string onset = string(tal, talOffset, onsetLength);
    talOffset += onsetLength;
    if (!validOnset(onset)) {
        cerr << "TAL onset has bad format. Skipping..." << endl;
        // TODO: need to write code to skip this TAL
        // not sure if anything needs to happen
    }
    return onset;
}

bool EDF::validOnset(string& s) {
    // onset must start with '+' or '-'
    if (s[0] != '+' && s[0] != '-')
        return false;
    
    // can contain at most one '.'
    // and only numerals after first dot character
    int dotIndex = -1;
    for(int i = 1; i < s.length(); i++) {
        if (s[i] == '.' && dotIndex < 0) {// is it a dot
            dotIndex = (int)i;
            if (dotIndex == (int) s.length() - 1)
                return false; // cannot specify the dot without the fractional second part i.e. "+1." is bad
        } else if (s[i] < '0' || s[i] > '9') // not a numeral
            return false;
    }
    
    return true;
}


string EDF::parseDuration(char* const &tal, int &talOffset, int onsetLength) {
    talOffset++; // skip 21 character
    int durationLength = 0;
    // find length of duration by checking for 20
    while (tal[talOffset + durationLength] != 20)
        durationLength++;
    
    // verify duration character set
    string duration = string(tal, talOffset, durationLength);
    talOffset += durationLength;
    if (!validDuration(duration)) {
        cerr << "TAL duration has bad format. Skipping..." << endl;
        // TODO: need to write code to skip this TAL
        // not sure if anything needs to happen
    }
    
    return duration;
}

bool EDF::validDuration(string& s) {
    // can contain at most one '.'
    // and only numerals after first dot character
    int dotIndex = -1;
    for(int i = 0; i < s.length();i++) {
        if (s[i] == '.' && dotIndex < 0) // is it a dot
            dotIndex = (int)i;
        else if (s[i] < '0' || s[i] > '9') // not a numeral
            return false;
    }
    
    return true;
}

string EDF::parseAnnotation(char* const &tal, int& talOffset) {
    talOffset++; // skip 20 character
    int annotLength = 0;
    // find length of annotation strings by searching for consecutive bytes {20, 0}
    while (tal[talOffset + annotLength] != 20 && tal[talOffset + annotLength + 1] != 0)
        annotLength++;
    
    string s(tal, talOffset, annotLength);
    talOffset += annotLength;
    tal[talOffset + 1] = 20; // this is needed because the string constructor will not read past a zero for indexing (ref line 309)
    talOffset += 2; // skip 20 and 0 character, we know there is a 0 character because the while loop above specifically checked for it
    
    return s;
}

std::vector<double>* EDF::parseSignal(std::fstream& in, int signal, double startTime, double length) {
    // you should check that the signal value is in range before calling this method
    if (startTime < 0 || startTime > recordingTime) {
        cerr << "Signal start time out of range. Giving up..." << endl;
        return nullptr;
    }
    
    double freq = channels[signal].signalSampleCount / dataRecordDuration;
    //EDFSignalData* data = new EDFSignalData(freq, header->physicalMax(signal), header->physicalMin(signal));
    vector<double>* data = new vector<double>();
    
    // seek to beginning of data records
    in.seekg( signalCount * 256 + 256, std::ios::beg);
    
    char* record = new char[dataRecordSize]; // each record is the same size
    int startInRecordBuffer = channels[signal].bufferOffset;// header->bufferOffset(signal); // this is the byte where the channel starts in each record
    int endInRecordBuffer = startInRecordBuffer + channels[signal].signalSampleCount * 2; // this is the byte where the channel ends in each record
    
    int convertedSignalLength;
    
    // convert startTime in seconds to a starting record location
    int startRecord = floor(startTime / dataRecordDuration); // as record index
    // convert length in seconds to an ending record location
    int endRecord = startRecord + ceil(length / dataRecordDuration); // as record index
    // truncate if beyond length of file
    if (endRecord > dataRecordCount)
        endRecord = dataRecordCount;
    
    // seek to beginning of first record to read
    in.seekg(signalCount * 256 + 256 + dataRecordSize * startRecord);
    
    int numberOfSamples = static_cast<int>(floor((startTime + length) * freq));
    for(auto recordNum = startRecord; recordNum < endRecord; recordNum++) {
        if(!in.read(record, dataRecordSize)) {
            cerr << "Error reading signal records from file. Giving up..." << endl;
            delete data;
            return nullptr;
        }
        
        // reading, possibly partial, first record
        if (recordNum == startRecord) {
            // convert from 2's comp to integers
            int start = static_cast<int>(floor(startTime * freq)) % channels[signal].signalSampleCount; // in samples
            int end = std::min((endInRecordBuffer - startInRecordBuffer) / 2, numberOfSamples); // in samples
            convertedSignalLength = end - start;
            
            for (int i = start; i < end; i++) {
                short raw = record[startInRecordBuffer + 2 * i] + 256 * record[startInRecordBuffer + 2 * i + 1];
                //short raw = (((short)record[startInRecordBuffer + 2 * i]) << 8) | (0x00ff & record[startInRecordBuffer + 2 * i + 1]);
                double value = parseSignalValue(raw, channels[signal]);//record[startInRecordBuffer + 2 * i] + 256 * record[startInRecordBuffer + 2 * i + 1];
                data->push_back(value);
            }
            // read the rest of the records from 0 to end
        } else {
            // convert from 2's comp to integers
            int end = std::min((endInRecordBuffer - startInRecordBuffer) / 2, numberOfSamples); // in bytes
            convertedSignalLength = end;
            for (int i = 0; i < end; i++) {
                short raw = record[startInRecordBuffer + 2 * i] + 256 * record[startInRecordBuffer + 2 * i + 1];
                //short raw = (((short)record[startInRecordBuffer + 2 * i]) << 8) | (0x00ff & record[startInRecordBuffer + 2 * i + 1]);
                double value = parseSignalValue(raw, channels[signal]);// = record[startInRecordBuffer + 2 * i] + 256 * record[startInRecordBuffer + 2 * i + 1];
                data->push_back(value);
                //cout << raw << " " << value << endl;
            }
        }
        
        //data->addDataPoints(convertedSignal, convertedSignalLength);
        numberOfSamples -= channels[signal].signalSampleCount;// header->signalSampleCount(signal); // trim for 'end' calculations
        
        
    }
    
    delete [] record;
    
    return data;
}

double EDF::parseSignalValue(short raw, edf_channel_info channel) {
    //cout << channel.digitalMax  << " " << channel.digitalMin << " " << channel.physicalMax  << " " << channel.physicalMin << endl;
    double convertionFactor = 1 * ((channel.physicalMax - channel.physicalMin)/( channel.digitalMax - channel.digitalMin) );
    int intRow = static_cast<int>(raw);// + 32767;
    double value = (double) (static_cast<double>(intRow) * convertionFactor);
    //cout << channel.label << "  " << value << " " << (powerVolts / ADCbits) * value << endl;
    return /*(powerVolts / ADCbits) * */ value;
}

string convertSpaces(const string& str) {
    string s = str;
    size_t spaceLoc = s.find(" ");
    while (spaceLoc != string::npos) {
        s.replace(spaceLoc, 1, "_");
        spaceLoc = s.find(" ", spaceLoc + 1);
    }
    return s;
};

string convertUnderscores(const string& str) {
    string s = str;
    size_t underLoc = s.find("_");
    while (underLoc != string::npos) {
        s.replace(underLoc, 1, " ");
        underLoc = s.find("_", underLoc + 1);
    }
    return s;
};

//trim spaces from string
string trim(string str) {
    // operate on copy, not on reference
    size_t pos = str.find_last_not_of(' ');
    if(pos != string::npos) {
        str.erase(pos + 1);
        pos = str.find_first_not_of(' ');
        if(pos != string::npos)
            str.erase(0, pos);
    } else
        str.erase(str.begin(), str.end());
    
    return str;
};

bool charactersValid(char *str, int len) {
    range_loop(i, 0, len, 1) {
        if (str[i] < 32 || str[i] > 126) {
            cerr << "Invalid characters detected in header" << endl;
            return false;
        }
    }
    
    return true;
};
