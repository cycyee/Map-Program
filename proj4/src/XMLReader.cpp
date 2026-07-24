#include "XMLReader.h"
#include"XMLEntity.h"
#include "DataSource.h"
#include<string.h>
#include<cstddef>
#include<iostream>
#include <expat.h>
#include "queue"

struct CXMLReader::SImplementation {
    std::shared_ptr< CDataSource > DDataSource;
    XML_Parser DXMLParser;
    std::queue<SXMLEntity> DEntityQueue;
    bool skipcdata;
    
    void StartElementHandler(const std::string &name, const std::vector<std::string> &attrs) {
        SXMLEntity TempEntity;
        TempEntity.DNameData = name;
        TempEntity.DType = SXMLEntity::EType::StartElement;
        for(size_t Index = 0; Index < attrs.size(); Index += 2) { //to add values in as pairs
            TempEntity.SetAttribute(attrs[Index], attrs[Index + 1]); // Loop through attributes and add them to the entity
        }
        DEntityQueue.push(TempEntity); // Enqueue the entity
    }

    void EndElementHandler(const std::string &name) {
        SXMLEntity entity;
        entity.DNameData = name;
        entity.DType = SXMLEntity::EType::EndElement; // Mark as the end element
        DEntityQueue.push(entity);
    }


    void CharacterDataHandler(const std::string &cdata) {
        // if (!cdata.empty()) { 
        //     SXMLEntity entity;
        //     entity.DNameData = cdata; // give the character data
        //     entity.DType = SXMLEntity::EType::CharData; //Mark as character data
        //     DEntityQueue.push(entity);
        // }
        if (!cdata.empty()) {
            if (!DEntityQueue.empty() && DEntityQueue.back().DType == SXMLEntity::EType::CharData) {
            // Concatenate CDATA with the last entity if it is also CharData
            DEntityQueue.back().DNameData += cdata;
            } else {
                SXMLEntity entity;
                entity.DNameData = cdata;
                entity.DType = SXMLEntity::EType::CharData;
                DEntityQueue.push(entity);
            }
        }
    }

    static void StartElementHandlerCallback(void *context, const XML_Char *name, const XML_Char **atts) {
        SImplementation *ReaderObject = static_cast<SImplementation *>(context);
        std::vector<std::string> Attributes;  // Temporary vector to hold attributes
        auto AttrPtr = atts;
        while(*AttrPtr) {
            Attributes.push_back(*AttrPtr);
            AttrPtr++;
        }
        ReaderObject -> StartElementHandler(name, Attributes); // Call
    }


    static void EndElementHandlerCallback(void *context, const XML_Char *name) {
        SImplementation *ReaderObject = static_cast<SImplementation *>(context);
        ReaderObject->EndElementHandler(name); //call end handler
    };

    static void CharacterDataHandlerCallback (void *context, const XML_Char *s, int len) {
        SImplementation *ReaderObject = static_cast<SImplementation*>(context);
        ReaderObject -> CharacterDataHandler(std::string(s, len)); // call char handler
    };
    
    SImplementation(std::shared_ptr< CDataSource > src) {
        DDataSource = src; 
        DXMLParser = XML_ParserCreate(NULL);
        XML_SetStartElementHandler(DXMLParser, StartElementHandlerCallback);
        XML_SetEndElementHandler(DXMLParser, EndElementHandlerCallback);
        XML_SetCharacterDataHandler(DXMLParser, CharacterDataHandlerCallback);
        XML_SetUserData(DXMLParser, this);
    };


    bool End() const {
        return (DEntityQueue.empty() && DDataSource->End());

    };

    bool ReadEntity(SXMLEntity &entity, bool skipcdata) {
        
        //read from source, pass to parser, return the entity
        // while(DEntityQueue.empty()&& !DDataSource->End()) {
        //     std::vector<char> DataBuffer;
        //     if (DDataSource ->Read(DataBuffer, 512)) {
        //         XML_Parse(DXMLParser, DataBuffer.data(), DataBuffer.size(), DataBuffer.size() < 512);
        //         break;
        //     }
        //     else {
        //         XML_Parse(DXMLParser, DataBuffer.data(), 0, true);
        //     }
        // }

        
        // if(DEntityQueue.empty()) {
        //     return false;
        // }
        // if(skipcdata == true) {//pop CDATA elements off if skipcdata is true
        //     for(size_t i = 0; i < DEntityQueue.size(); i++) {
        //         if(DEntityQueue.front().DType == SXMLEntity::EType::CharData){
        //             DEntityQueue.pop();
        //         }
        //     }
        // }
        // entity = DEntityQueue.front();
        // DEntityQueue.pop();
        // return true;
        // Pull data until the queue has something or the source is exhausted.
        // Each Read grabs a large block and we keep looping (refilling) until an
        // entity is produced, so inputs larger than one block parse fully.
        constexpr std::size_t ReadChunk = 1 << 20; // 1 MB per read
        while (DEntityQueue.empty() && !DDataSource->End()) {
            std::vector<char> DataBuffer;
            if (DDataSource->Read(DataBuffer, ReadChunk) && !DataBuffer.empty()) {
                XML_Parse(DXMLParser, DataBuffer.data(), DataBuffer.size(), DDataSource->End());
            } else {
                XML_Parse(DXMLParser, nullptr, 0, true); // Signal end of parsing
                break;
            }
        }

        if (DEntityQueue.empty()) {
            return false;
        }

        if (skipcdata) {
            // Skip CDATA entities
            while (!DEntityQueue.empty() && DEntityQueue.front().DType == SXMLEntity::EType::CharData) {
                DEntityQueue.pop();
            }
        }

        if (DEntityQueue.empty()) {
            return false;
        }

        entity = DEntityQueue.front();
        DEntityQueue.pop();
        return true;
    }
};

CXMLReader::CXMLReader(std::shared_ptr < CDataSource > src){
    DImplementation = std::make_unique< SImplementation >(src);
}

CXMLReader::~CXMLReader(){
}

bool CXMLReader::ReadEntity(SXMLEntity &entity, bool skipcdata){
    return DImplementation->ReadEntity(entity, skipcdata);
}

bool CXMLReader::End() const {
    return DImplementation->End();

}