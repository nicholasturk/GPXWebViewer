#include <stdbool.h>

#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "GPXParser.h"
#include "LinkedListAPI.h" 

char *GPXdocToString(GPXdoc *doc){

    if(!doc) return NULL;

    char *wp = toString(doc->waypoints);
    char *rts = toString(doc->routes);
    char *trks = toString(doc->tracks);

    int doubleLen = 3 + DBL_MANT_DIG - DBL_MIN_EXP;
    int len = strlen(wp)+strlen(rts)+strlen(trks)+strlen(doc->creator)+doubleLen+256;

    char *str = malloc(sizeof(char)*len);

    sprintf(str, "\ncreator: %s\nversion: %f\n\n--Waypoints--\n%s\n--Routes--\n%s\n\n--Tracks--\n%s\n",
            doc->creator,doc->version,wp,rts,trks);

    free(wp);
    free(rts);
    free(trks);

    return str;
}


char *getContent(xmlNode *toCheck){
    if(toCheck->children){
        if(toCheck->children->content){
            return (char*)toCheck->children->content;
        }
    }
    return "\0";
}

Waypoint *parseWayPoint(xmlNode *node){

	Waypoint *ret = malloc(sizeof(Waypoint));
    ret->name = NULL;
    ret->longitude = 0;
    ret->latitude = 0;
    ret->otherData = initializeList(gpxDataToString, deleteGpxData, compareGpxData);

    // Get attributes
	xmlAttr *attr;
	for (attr = node->properties; attr; attr = attr->next){
        
		xmlNode *value = attr->children;
		char *attrName = (char*)attr->name;
		char *cont = (char*)(value->content);

		if(strcmp((char*)attrName, "lat") == 0) ret->latitude = atof(cont);
		if(strcmp((char*)attrName, "lon") == 0) ret->longitude = atof(cont);
	}

	// Get waypoint children
	xmlNode *child;
	for (child = node->children; child; child = child->next){

		if (child->type == XML_ELEMENT_NODE){
            
			// Get waypoint name 
			if(strcmp((char*)child->name, "name") == 0){
                char *value = getContent(child);
				ret->name = malloc(sizeof(char) * (strlen(value) + 1));
                strncpy(ret->name, value, strlen(value) + 1);

			// Any other data
			} else {
				char *value = getContent(child);
				GPXData *other = malloc(sizeof(GPXData) + (sizeof(char) * (strlen(value) + 1)));

				strcpy(other->name, (char*)child->name);
				strcpy(other->value, value);

				insertBack(ret->otherData, other);
			}
		}
	}

    if(!ret->name){
        ret->name = malloc(sizeof(char));
        strcpy(ret->name, "\0");
    }

	return ret;
}

TrackSegment *parseTrackSeg(xmlNode *node, Track *currTrack){

	TrackSegment *currTrackSeg = malloc(sizeof(TrackSegment));
	currTrackSeg->waypoints = initializeList(waypointToString, deleteWaypoint, compareWaypoints);

	// Traverse <trkseg> 
	xmlNode *currNode = NULL;
	for (currNode = node; currNode; currNode = currNode->next) {

        // Make sure it's an XML_ELEMENT_NODE
        if (currNode->type == XML_ELEMENT_NODE) {

            // Find <trkpt> and add to waypoints list
            if (strcmp((char*)currNode->name, "trkpt") == 0) {
                Waypoint *temp = parseWayPoint(currNode);
                insertBack(currTrackSeg->waypoints, temp);
            }
        }
	}

    return currTrackSeg;
}

void parseTrack(xmlNode *trackNode, GPXdoc *doc){
    
    //Initialize the current <trk> element and add to list
    Track *currTrack = malloc(sizeof(Track));
    currTrack->name = NULL;
    currTrack->otherData = initializeList(gpxDataToString, deleteGpxData, compareGpxData);
    currTrack->segments = initializeList(trackSegmentToString, deleteTrackSegment, compareTrackSegments);
    insertBack(doc->tracks, currTrack);

    //Traverse <trk> element
    xmlNode *currNode = NULL;
    for(currNode = trackNode; currNode; currNode = currNode->next){

        //Make sure it's an XML_ELEMENT_NODE
        if(currNode->type == XML_ELEMENT_NODE){

            // Get <name> value
            if (strcmp((char*)currNode->name, "name") == 0){
                char *value = getContent(currNode);
                currTrack->name = malloc(sizeof(char) * (strlen(value) + 1));
                strncpy(currTrack->name, value, strlen(value) + 1);
            }

            // Get other GPXData 
            else if(strcmp((char*)currNode->name, "trkseg") != 0) {

                char *value = getContent(currNode);
                GPXData *other = malloc(sizeof(GPXData) + sizeof(char) * ((strlen(value) + 1)));

                strcpy(other->name, (char*)currNode->name);
                strcpy(other->value, value);

				insertBack(currTrack->otherData, other);

            // Get <trkseg> data
            } else{
				TrackSegment *temp = parseTrackSeg(currNode->children, currTrack);
                insertBack(currTrack->segments, temp);
            }
        }
    }
    if(!currTrack->name){
        currTrack->name = malloc(sizeof(char));
        strcpy(currTrack->name, "\0");
    }
}

void parseRoute(xmlNode *routeNode, GPXdoc *doc) {

    // Initialize the current <trk> element and add to list
    Route *currRoute = malloc(sizeof(Route));
    currRoute->name = NULL;
    currRoute->waypoints = initializeList(waypointToString, deleteWaypoint, compareWaypoints);
    currRoute->otherData = initializeList(gpxDataToString, deleteGpxData, compareGpxData);
    insertBack(doc->routes, currRoute);

    // Traverse <rte> element
    xmlNode *currNode = NULL;
    for (currNode = routeNode; currNode; currNode = currNode->next) {

        // Make sure it's an XML_ELEMENT_NODE
        if (currNode->type == XML_ELEMENT_NODE) {
            // Get <name> value
            if (strcmp((char*)currNode->name, "name") == 0){
                char *value = getContent(currNode);
                currRoute->name = malloc(sizeof(char) * (strlen(value) + 1));
                strcpy(currRoute->name, value);
            }

            // Get other GPXData
            else if (strcmp((char*)currNode->name, "rtept") != 0) {
                char *value = getContent(currNode);
                GPXData *other = malloc(sizeof(GPXData) + sizeof(char) * (strlen(value) + 1));

                strcpy(other->name, (char*)currNode->name);
                strcpy(other->value, value);

                insertBack(currRoute->otherData, other);

            // Parse <rtpt> and add to list
            } else {
                Waypoint *temp = parseWayPoint(currNode);
                insertBack(currRoute->waypoints, temp);
            }
        }
    }
    if(!currRoute->name){
        currRoute->name = malloc(sizeof(char));
        strcpy(currRoute->name, "\0");
    }
}

void traverseRoot(xmlNode *node, GPXdoc *doc){

    // Iterate root node
    xmlNode *curNode;
    for (curNode = node; curNode; curNode = curNode->next){

        // Ensure it's a XML_ELEMENT_NODE
        if (curNode->type == XML_ELEMENT_NODE){

            // Get <gpx> data
            if(strcmp((char*)curNode->name, "gpx") == 0){

                if(strlen(doc->namespace) == 0){
                    if (curNode->ns){
                        if(curNode->ns->href){
                            strcpy(doc->namespace, (char*)curNode->ns->href);
                        }
                    }
                }

                // Iterate attributes of gpx and find creator / version
                xmlAttr *attr;
                for(attr = curNode->properties; attr != NULL; attr = attr->next) {

                    xmlNode *value = attr->children;
                    char *attrName = (char *)attr->name;
                    char *cont = (char *)(value->content);
                    
                    if(strcmp((char*)attrName, "creator") == 0){
                        doc->creator = malloc(sizeof(char) * (strlen(cont) + 1));
                        strcpy(doc->creator, cont);
                    }else if(strcmp((char*)attrName, "version") == 0) {
                        doc->version = atof(cont);
                    }
                }
            
            // Get <trk> data
            } else if(strcmp((char*)curNode->name, "trk") == 0){
                parseTrack(curNode->children, doc);
            }

            // Get <rte> data
            else if(strcmp((char*)curNode->name, "rte") == 0){
                parseRoute(curNode->children, doc);
            }

            // Get <wpt> data
			else if (strcmp((char*)curNode->name, "wpt") == 0){
				Waypoint *temp = parseWayPoint(curNode);
				insertBack(doc->waypoints, temp);
			}
        }
        traverseRoot(curNode->children, doc);
    }
}

void deleteGPXdoc(GPXdoc *doc) {

    if (!doc) return;

    freeList(doc->waypoints);
    freeList(doc->tracks);
    freeList(doc->routes);
    free(doc->creator);
    free(doc);
}

int getNumRoutes(const GPXdoc *doc) {
    if (!doc) return 0;
    if (!doc->routes) return 0;
    return doc->routes->length;
}

int getNumTracks(const GPXdoc *doc) {
    if (!doc) return 0;
    if (!doc->tracks) return 0;
    return doc->tracks->length;
}

int getNumWaypoints(const GPXdoc *doc){
    if (!doc) return 0;
    if(!doc->waypoints) return 0;
    return doc->waypoints->length;
}

int getNumSegments(const GPXdoc *doc) {
    if (!doc) return 0;
    if(!doc->tracks) return 0;

    int count = 0;
    ListIterator trackIter = createIterator(doc->tracks);
    Track *trackPtr;

    while ((trackPtr = (Track *)nextElement(&trackIter)) != NULL) {
        if(trackPtr->segments) count += trackPtr->segments->length;
    }

    return count;
}

int parseWpOtherData(List *wps){
    if(!wps) return 0;
    int count = 0;
    Waypoint *waypointPtr;
    ListIterator waypointsIter = createIterator(wps);
    while ((waypointPtr = (Waypoint *)nextElement(&waypointsIter)) != NULL) {
        if (waypointPtr->otherData) count += waypointPtr->otherData->length;
        if (strcmp(waypointPtr->name, "") != 0) count++;
    }
    return count;
}

int getNumGPXData(const GPXdoc *doc) {
    if (!doc) return 0;

    int count = 0;
    if (doc->waypoints) count += parseWpOtherData(doc->waypoints);

    ListIterator trackIter = createIterator(doc->tracks);
    ListIterator routeIter = createIterator(doc->routes);

    Track *trackPtr;
    TrackSegment *segPtr;
    Route *routePtr;

    // Iterate tracks to get points
    while ((trackPtr = (Track *)nextElement(&trackIter)) != NULL) {
        if(trackPtr->otherData) count += trackPtr->otherData->length;
        if(strcmp(trackPtr->name, "") != 0) count ++;
        ListIterator segIter = createIterator(trackPtr->segments);
        while ((segPtr = (TrackSegment *)nextElement(&segIter)) != NULL) {
            if (segPtr->waypoints) {
                count += parseWpOtherData(segPtr->waypoints);
            }
        }
    }
    while ((routePtr = (Route *)nextElement(&routeIter)) != NULL) {
        if (routePtr->waypoints) count += parseWpOtherData(routePtr->waypoints);
        if (routePtr->otherData) count += routePtr->otherData->length;
        if (strcmp(routePtr->name, "") != 0) count++;
    }

    return count;
}

Track *getTrack(const GPXdoc *doc, char *name){
    if (!doc) return NULL;
    if (!name) return NULL;

    ListIterator trackIter = createIterator(doc->tracks);
    Track *trackPtr;

    while ((trackPtr = (Track *)nextElement(&trackIter)) != NULL) {
        if (strcmp(trackPtr->name, name) == 0) return trackPtr;
    }

    return NULL;
}

Route *getRoute(const GPXdoc *doc, char *name) {
    if (!doc) return NULL;
    if (!name) return NULL;

    ListIterator routerIter = createIterator(doc->routes);
    Route *routePtr;

    while ((routePtr = (Route *)nextElement(&routerIter)) != NULL) {
        if (strcmp(routePtr->name, name) == 0) return routePtr;
    }

    return NULL;
}

Waypoint *getWaypoint(const GPXdoc *doc, char *name){
    if (!doc) return NULL;
    if (!doc->waypoints) return NULL;

    ListIterator wps = createIterator(doc->waypoints);
    Waypoint *waypointPtr;

    while( (waypointPtr = (Waypoint*)nextElement(&wps)) != NULL ){
        if(waypointPtr->name){
            if(strcmp(waypointPtr->name, name) == 0){
                return waypointPtr;
            }
        }
    }
    return NULL;
}

char *gpxDataToString(void* data) {
    if(!data) return NULL;

    char *temp;
    int len;
    GPXData *ptr = (GPXData*)data;

    len = strlen(ptr->name)+strlen(ptr->value)+6;
    temp = malloc(sizeof(char)*len);

    sprintf(temp, "\t%s: %s\n", ptr->name, ptr->value);
    return temp;

}

void deleteGpxData(void *data){
    if(!data) return;
    GPXData *ptr = (GPXData*)data;
    free(ptr);
}

char *waypointToString(void *data){
    if(!data) return NULL;

    char *ret;
    int len;
    int maxFloatLength = 3 + DBL_MANT_DIG - DBL_MIN_EXP;
    Waypoint *ptr = (Waypoint*)data;

    char *od = toString(ptr->otherData);

    len = strlen(ptr->name)+strlen(od)+maxFloatLength*2+45;
    ret = malloc(sizeof(char)*len);

    len = sprintf(ret, "\n\tname: %s, lon: %f, lat: %f\n\tother:\n%s", ptr->name, 
            ptr->longitude, ptr->latitude, od);

    free(od);

    return ret;
}

void deleteWaypoint(void *data) {
    if (!data)  return;
    Waypoint *ptr = (Waypoint*)data;
    freeList(ptr->otherData);
    free(ptr->name);
    free(ptr);
}

char *routeToString(void *data){
    if(!data) return NULL;
    Route *ptr = (Route*)data;
    char *wp = toString(ptr->waypoints);
    char *od = toString(ptr->otherData);

    int len = strlen(wp)+strlen(od)+strlen(ptr->name)+45;
    char *ret = malloc(sizeof(char)*len);
    sprintf(ret, "\n------%s------\n\nWaypoints:\n%sOtherData:\n%s", ptr->name, wp, od);

    free(wp);
    free(od);

    return ret;
}


void deleteRoute(void *data){
    if (!data) return;
    Route *ptr = (Route*)data;
    freeList(ptr->otherData);
    freeList(ptr->waypoints);
    free(ptr->name);
    free(ptr);
}

char *trackSegmentToString(void* data){
    if (!data) return NULL;

    TrackSegment *ptr = (TrackSegment*)data;
    char *wp = toString(ptr->waypoints);
    int len = strlen(wp)+12;
    char *ret = malloc(sizeof(char)*len);

    sprintf(ret, "\nSegment:\n%s", wp);
    free(wp);
    return ret;
}


void deleteTrackSegment(void *data){
    if (!data) return;
    TrackSegment *ptr = (TrackSegment*)data;
    freeList(ptr->waypoints);
    free(ptr);
}

char *trackToString(void *data){

    if(!data) return NULL;
    Track *ptr = (Track*)data;

    char *segs = toString(ptr->segments);
    char *od = toString(ptr->otherData);

    int len = strlen(segs)+strlen(od)+strlen(ptr->name)+40;
    char *ret = malloc(sizeof(char)*len);
    sprintf(ret, "\n\n------%s------\n%s\nOtherData:\n%s", ptr->name, segs, od);

    free(segs);
    free(od);

    return ret;
}


void deleteTrack(void *data){
    if (!data) return;
    Track *ptr = (Track*)data;
    freeList(ptr->otherData);
    freeList(ptr->segments);
    free(ptr->name);
     free(ptr);
}

// Useless compare functions
int compareGpxData(const void *first, const void *second) { return 0; }
int compareWaypoints(const void *first, const void *second) { return 0; }
int compareRoutes(const void *first, const void *second) { return 0; }
int compareTrackSegments(const void *first, const void *second) { return 0; }
int compareTracks(const void *first, const void *second) { return 0; }

GPXdoc *createGPXdoc(char *fileName) {

    // Initialize xmlReader
    xmlDoc *doc = NULL;
    xmlNode *rootElement = NULL;
    doc = xmlReadFile(fileName, NULL, 0);

    if (!fileName || fileName[0] == '\0' || !doc){
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    // Initialize GPXdoc
    GPXdoc *ret = NULL;
    ret = malloc(sizeof(GPXdoc));

    strcpy(ret->namespace, "");
    ret->waypoints =
        initializeList(waypointToString, deleteWaypoint, compareWaypoints);
    ret->routes = initializeList(routeToString, deleteRoute, compareRoutes);
    ret->tracks = initializeList(trackToString, deleteTrack, compareTracks);

    rootElement = xmlDocGetRootElement(doc);
    traverseRoot(rootElement, ret);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return ret;
}

void addOtherData(xmlNodePtr parent, List *otherData, xmlNsPtr ns){

    ListIterator otherDataIter = createIterator(otherData);
    GPXData *otherDataPtr;

    while ((otherDataPtr = (GPXData *)nextElement(&otherDataIter)) != NULL) {
        xmlNewChild(parent, ns, BAD_CAST otherDataPtr->name,
                    BAD_CAST otherDataPtr->value);
    }
}


void addWayPoints(xmlNodePtr parent, List *waypoints, char *waypointType, xmlNsPtr ns){

    xmlNodePtr currWaypointNode = NULL;

    ListIterator waypointIter = createIterator(waypoints);
    Waypoint *waypointPtr;

    while ((waypointPtr = (Waypoint *)nextElement(&waypointIter)) != NULL) {

        currWaypointNode = xmlNewNode(ns, BAD_CAST waypointType);

        if (waypointPtr->otherData->length > 0)
            addOtherData(currWaypointNode, waypointPtr->otherData, ns);

        if (strlen(waypointPtr->name) > 0)
            xmlNewChild(currWaypointNode, ns, BAD_CAST "name",
                        BAD_CAST waypointPtr->name);

        int size = snprintf(0,0,"%f",waypointPtr->latitude);
        if (size >= 0){
            char buf[size+1];
            sprintf(buf, "%f", waypointPtr->latitude);
            xmlNewProp(currWaypointNode, BAD_CAST "lat", BAD_CAST buf);
        }

        size = snprintf(0, 0, "%f", waypointPtr->longitude);
        if (size >= 0) {
            char buf[size + 1];
            sprintf(buf, "%f", waypointPtr->longitude);
            xmlNewProp(currWaypointNode, BAD_CAST "lon", BAD_CAST buf);
        }
        xmlAddChild(parent, currWaypointNode);
    }
}

void addTracks(xmlNodePtr root, List *tracks, xmlNsPtr ns){

    if (tracks->length == 0) return;

    xmlNodePtr currTrackNode = NULL, currTrackSegNode = NULL;
    
    ListIterator trackIter = createIterator(tracks);
    Track *trackPtr;

    while ((trackPtr = (Track *)nextElement(&trackIter)) != NULL) {

        currTrackNode = xmlNewNode(ns, BAD_CAST "trk");
        if(strlen(trackPtr->name) > 0) 
            xmlNewChild(currTrackNode, ns, BAD_CAST "name", BAD_CAST trackPtr->name);

        addOtherData(currTrackNode, trackPtr->otherData, ns);

        if (trackPtr->segments->length > 0){

            ListIterator segIter = createIterator(trackPtr->segments);
            TrackSegment *segPtr;

            while ((segPtr = (TrackSegment *)nextElement(&segIter)) != NULL){
                currTrackSegNode = xmlNewNode(ns, BAD_CAST "trkseg");
                addWayPoints(currTrackSegNode, segPtr->waypoints, "trkpt", ns);
                xmlAddChild(currTrackNode, currTrackSegNode);
            }

        }
        xmlAddChild(root, currTrackNode);
    }
}

void addRoutes(xmlNodePtr root, List *routes, xmlNsPtr ns) {
    if (routes->length == 0) return;

    xmlNodePtr currRouteNode = NULL;

    ListIterator routeIter = createIterator(routes);
    Route *routePtr;

    while ((routePtr = (Route *)nextElement(&routeIter)) != NULL) {
        currRouteNode = xmlNewNode(ns, BAD_CAST "rte");
        if (strlen(routePtr->name) > 0)
            xmlNewChild(currRouteNode, ns, BAD_CAST "name",
                        BAD_CAST routePtr->name);

        addOtherData(currRouteNode, routePtr->otherData, ns);
        addWayPoints(currRouteNode, routePtr->waypoints, "rtept", ns);
        xmlAddChild(root, currRouteNode);
    }
}

xmlDoc *createXmlDoc(GPXdoc *gDoc){

    if (!gDoc) return NULL;

    xmlDocPtr xDoc = NULL;
    xmlNodePtr rootNode = NULL;  
    xmlNsPtr nsPtr = NULL;  

    xDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewNode(NULL, BAD_CAST "gpx");

    nsPtr = xmlNewNs(rootNode, BAD_CAST gDoc->namespace, NULL);
    xmlSetNs(rootNode, nsPtr);
    

    if(gDoc->creator){
        xmlNewProp(rootNode, BAD_CAST "creator", BAD_CAST gDoc->creator);
    }

    int size = snprintf(0, 0, "%.1f", gDoc->version);
    if (size >= 0) {
        char buf[size + 1];
        sprintf(buf, "%.1f", gDoc->version);
        xmlNewProp(rootNode, BAD_CAST "version", BAD_CAST buf);
    }

    xmlDocSetRootElement(xDoc, rootNode);

    // Add waypoints, tracks and routes to rootNode
    addWayPoints(rootNode, gDoc->waypoints, "wpt", nsPtr);
    addRoutes(rootNode, gDoc->routes, nsPtr);
    addTracks(rootNode, gDoc->tracks, nsPtr);

    return xDoc;
}

bool validateOtherData(List *otherData) {

    if(!otherData) return false;

    ListIterator otherDataIter = createIterator(otherData);
    GPXData *otherDataPtr;

    while ((otherDataPtr = (GPXData *)nextElement(&otherDataIter)) != NULL) {
        if (otherDataPtr->value[0] == '\0') return false;
    }

    return true;
}

bool validateWaypoints(List *waypoints) {

    if(!waypoints) return false;

    ListIterator waypointIter = createIterator(waypoints);
    Waypoint *waypointPtr;

    while ((waypointPtr = (Waypoint *)nextElement(&waypointIter)) != NULL) {
        if (!waypointPtr->name) return false;
        if (!waypointPtr->longitude || !waypointPtr->latitude) return false;
        if(!validateOtherData(waypointPtr->otherData)) return false;
    }

    return true;

}

bool validateRoutes(List *routes) {

    ListIterator routeIter = createIterator(routes);
    Route *routePtr;

    while ((routePtr = (Route *)nextElement(&routeIter)) != NULL) {
        if (!routePtr->name) return false;
        if (!validateOtherData(routePtr->otherData)) return false;
        if (!validateWaypoints(routePtr->waypoints)) return false;
    }

    return true;
}

bool validateTracks(List *tracks) {

    ListIterator trackIter = createIterator(tracks);
    Track *trackPtr;

    while ((trackPtr = (Track *)nextElement(&trackIter)) != NULL) {

        if (!trackPtr->name) return false;
        if(!trackPtr->segments) return false;

        if (!validateOtherData(trackPtr->otherData)) return false;

        ListIterator segIter = createIterator(trackPtr->segments);
        TrackSegment *segPtr;

        while ((segPtr = (TrackSegment *)nextElement(&segIter)) != NULL) {
            if(!validateWaypoints(segPtr->waypoints)) return false;
        }
    }

    return true;
}

bool validateSpec(GPXdoc *doc){

    if (doc->namespace[0] == '\0') return false;
    if (!doc->creator || doc->creator[0] == '\0') return false;
    if (!(doc->version)) return false;
    if(!(doc->waypoints)) return false;
    if(!(doc->routes)) return false;
    if (!(doc->tracks)) return false;

    if(!validateWaypoints(doc->waypoints)) return false;
    if(!(validateTracks(doc->tracks))) return false;
    if(!(validateRoutes(doc->routes))) return false;

    return true;
}

bool writeGPXdoc(GPXdoc *doc, char *fileName){

    if(!doc) return false;
    if(!fileName || fileName[0] == '\0') return false;
    if(strlen(fileName) > 4){
        if( strcmp(fileName + strlen(fileName) - 4, ".gpx") != 0 ){
            printf("Line 792\n");
            return false;
        }
    } else{
        printf("Line 796\n");
        return false;
    }

    xmlDocPtr xDoc = NULL;

    int ret;
    xDoc = createXmlDoc(doc);
    ret = xmlSaveFormatFileEnc(fileName, xDoc, "UTF-8", 1);

    xmlFreeDoc(xDoc);
    xmlCleanupParser();

    return ret == -1 ? false : true;


}

bool validateGPXDoc(GPXdoc *doc, char *gpxSchemaFile){

    if(!doc || !gpxSchemaFile || gpxSchemaFile[0] == '\0') return false;
    if(!validateSpec(doc)) return false;

    xmlDocPtr xDoc = NULL;
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr ctxt;

    // Construct schema
    xmlLineNumbersDefault(1);
    ctxt = xmlSchemaNewParserCtxt(gpxSchemaFile);
    // xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc)fprintf,
    //                          (xmlSchemaValidityWarningFunc)fprintf, stderr);
    schema = xmlSchemaParse(ctxt);
    xmlSchemaFreeParserCtxt(ctxt);

    // Get xmlDoc
    xDoc = createXmlDoc(doc);
    if (!xDoc) return false;

    xmlSchemaValidCtxtPtr ctxtValid;
    int ret;

    ctxtValid = xmlSchemaNewValidCtxt(schema);
    // xmlSchemaSetValidErrors(ctxtValid, (xmlSchemaValidityErrorFunc)fprintf,
    //                         (xmlSchemaValidityWarningFunc)fprintf, stderr);

    // Validate schema context vs xmlDoc
    ret = xmlSchemaValidateDoc(ctxtValid, xDoc);

    xmlSchemaFreeValidCtxt(ctxtValid);
    xmlFreeDoc(xDoc);

    if (schema != NULL) xmlSchemaFree(schema); 
    
    xmlSchemaCleanupTypes();
    xmlCleanupParser();

    if (ret == 0) return true;

    return false;

}

GPXdoc *createValidGPXdoc(char *fileName, char *gpxSchemaFile){

    // Check for NULL or empty fileName
    if (!fileName || fileName[0] == '\0') return NULL;
    if (!gpxSchemaFile || gpxSchemaFile[0] == '\0') return NULL;
    
    
    GPXdoc *doc = createGPXdoc(fileName);
    if (!doc) return NULL;

    if (!validateGPXDoc(doc, gpxSchemaFile)) {
        deleteGPXdoc(doc);
        return NULL;
    }
    return doc;
}

float round10(float len){
    if (len < 0) return 0;
    int intLen = (int) len;
    int rem  = intLen % 10;
    return rem < 5.0 ? intLen - rem : intLen + (10 - rem);
}

float waypointDist(float startLat, float startLon, float endLat, float endLon) {

    double R = 6371e3; 

    double startLatTemp = (M_PI / 180) * (startLat);
    double endLatTemp = (M_PI / 180) * (endLat);

    double differenceLon = (M_PI / 180) * (endLon - startLon);
    double differenceLat = (M_PI / 180) * (endLat - startLat);

    double a = sin(differenceLat / 2) * sin(differenceLat / 2) +
               cos(startLatTemp) * cos(endLatTemp) * sin(differenceLon / 2) *
               sin(differenceLon / 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double distance = R * c;

    return distance;
}

float getWaypointLength(List *waypoints){

    if (!waypoints) return -1;

    ListIterator wpIter = createIterator(waypoints);
    Waypoint *curr = (Waypoint *)nextElement(&wpIter),
             *nxt = (Waypoint *)nextElement(&wpIter);

    float sum = 0;

    while(nxt){
        sum += waypointDist(curr->latitude, curr->longitude, 
                            nxt->latitude, nxt->longitude);

        curr = nxt;
        nxt = (Waypoint *)nextElement(&wpIter);
    }
    return sum;
}

float getRouteLen(const Route *rt){

    if (!rt) return 0;
    return getWaypointLength(rt->waypoints);
}

float getTrackLen(const Track *tr) {

    if (!tr) return 0;

    float sum = 0;
    ListIterator segIter = createIterator(tr->segments);
    TrackSegment *curr = NULL, *last = NULL;

    Waypoint *lastWp = NULL, *firstWp = NULL;

    while((curr = (TrackSegment *)nextElement(&segIter)) != NULL){
        if(last){
            lastWp = getFromBack(last->waypoints);
            firstWp = getFromFront(curr->waypoints);

            sum += waypointDist(firstWp->latitude, firstWp->longitude, 
                                lastWp->latitude, lastWp->longitude);
        }

        sum += getWaypointLength(curr->waypoints);
        last = curr;
    }
    return sum;
}

int numRoutesWithLength(const GPXdoc *doc, float len, float delta){

    if (!doc || len < 0 || delta < 0) return 0;
    int count = 0;

    ListIterator routeIter = createIterator(doc->routes);
    Route *ptr = NULL;

    while ((ptr = (Route *)nextElement(&routeIter)) != NULL ){
        float routeLen = getRouteLen(ptr);
        if (fabs(routeLen - len) <= delta) count++;
    }
    return count;
}

int numTracksWithLength(const GPXdoc *doc, float len, float delta) {
    if (!doc || len < 0 || delta < 0) return 0;
    int count = 0;

    ListIterator trackIter = createIterator(doc->tracks);
    Track *ptr = NULL;

    while ((ptr = (Track *)nextElement(&trackIter)) != NULL) {
        float routeLen = getTrackLen(ptr);
        if (fabs(routeLen - len) <= delta) count++;
    }
    return count;
}

bool isLoopRoute(const Route *rt, float delta){

    if(!rt || delta < 0) return false;
    if(!rt->waypoints) return false;
    if(rt->waypoints->length < 4) return false;

    Waypoint *first = getFromFront(rt->waypoints);
    Waypoint *last = getFromBack(rt->waypoints);

    float dist = waypointDist(first->latitude, first->longitude,
                              last->latitude, last->longitude);

    return (fabs(dist) < delta);
}

bool isLoopTrack(const Track *tr, float delta){

    if(!tr || delta < 0) return false;
    if(!tr->segments) return false;

    int count = 0;
    ListIterator segIter = createIterator(tr->segments);
    TrackSegment *segPtr = NULL;

    while ( (segPtr = (TrackSegment *)nextElement(&segIter)) != NULL ){
        if (segPtr->waypoints) count += segPtr->waypoints->length; 
    }

    if (count < 4) return false;

    TrackSegment *firstSeg = NULL, *lastSeg = NULL;
    Waypoint *firstWp = NULL, *lastWp = NULL;

    firstSeg = getFromFront(tr->segments);
    lastSeg = tr->segments->length > 1 ? getFromBack(tr->segments) : firstSeg;

    firstWp = getFromFront(firstSeg->waypoints);
    lastWp = getFromBack(lastSeg->waypoints);

    float dist = waypointDist(firstWp->latitude, firstWp->longitude,
                              lastWp->latitude, lastWp->longitude);

    return (fabs(dist) < delta);
}

void deleteDummy(void *data){ return; }

List *getRoutesBetween(const GPXdoc *doc, float sourceLat, float sourceLong,
                        float destLat, float destLong, float delta) {

    if(!doc) return NULL;
    if(!doc->routes) return NULL;

    List *newList = initializeList(routeToString, deleteDummy, compareRoutes);
    ListIterator routeIter = createIterator(doc->routes);
    Route *routePtr = NULL;

    // Find starting end routes
    while ((routePtr = (Route *)nextElement(&routeIter)) != NULL){
        if(routePtr->waypoints){
            Waypoint *first = getFromFront(routePtr->waypoints);
            Waypoint *last = getFromBack(routePtr->waypoints);

            float startDist = waypointDist(first->latitude, first->longitude,
                                            sourceLat, sourceLong);
            float endDist = waypointDist(last->latitude, last->longitude,
                                            destLat, destLong);

            if (startDist <= delta && endDist <= delta){
                insertBack(newList, routePtr);
            }
        }
    }
    if(newList->length == 0){
        freeList(newList);
        return NULL;
    }
    return newList;
}

List *getTracksBetween(const GPXdoc *doc, float sourceLat, float sourceLong,
                       float destLat, float destLong, float delta) {

    if (!doc) return NULL;
    if (!doc->tracks) return NULL;

    List *newList = initializeList(trackToString, deleteDummy, compareTracks);
    Track *trackPtr = NULL;
    Waypoint *wpPtrFirst = NULL, *wpPtrLast = NULL;
    TrackSegment *trkSegFirst = NULL, *trkSegLast = NULL;

    ListIterator trackIter;
    trackIter = createIterator(doc->tracks);

    while( (trackPtr = (Track*)nextElement(&trackIter)) != NULL ){
        if(!trackPtr->segments) continue;

        trkSegFirst = getFromFront(trackPtr->segments);
        trkSegLast = getFromBack(trackPtr->segments);

        if(!trkSegFirst->waypoints) continue;
        wpPtrFirst = getFromFront(trkSegFirst->waypoints);
        wpPtrLast = getFromBack(trkSegLast->waypoints);

        float startDist = waypointDist(wpPtrFirst->latitude, wpPtrFirst->longitude,
                                        sourceLat, sourceLong);
        float endDist = waypointDist(wpPtrLast->latitude, wpPtrLast->longitude, 
                                        destLat, destLong);

        if (startDist <= delta && endDist <= delta){
            insertBack(newList, trackPtr);
        }
    }
    if(newList->length == 0){
        freeList(newList);
        return NULL;
    } else{
        return newList;
    }
}

int sizeOfInt(int num) { return snprintf(NULL, 0, "%d", num); }
int sizeOfFloat(float num) { return snprintf(NULL, 0, "%f", num); }
int sizeOfDouble(double num) { return snprintf(NULL, 0, "%f", num); }

char *trackToJSON(const Track *tr){

    if (!tr) {
        char *empty = malloc(sizeof(char)*3);
        strcpy(empty, "{}");
        return empty;
    }

    int numPoints = 0;
    ListIterator segIter;
    TrackSegment *segPtr;
    segIter = createIterator(tr->segments);

    while ((segPtr = (TrackSegment*)nextElement(&segIter)) != NULL){
        numPoints += segPtr->waypoints ? segPtr->waypoints->length : 0;
    }

    char *name = (tr->name && tr->name[0] != '\0') ? tr->name : "None";
    char *isLoop = isLoopTrack(tr, 10) ? "true" : "false";
    float len = round10(getTrackLen(tr));
    char *otherData = gpxDataListToJSON(tr->otherData);
    char *str = malloc(sizeof(char) * (strlen(name)+sizeOfFloat(len)+strlen(isLoop) + 200));

    sprintf(str, "{\"name\":\"%s\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s,\"otherData\":%s}", name, numPoints, len, isLoop, otherData);

    return str;

}

char *routeToJSON(const Route *rt){
    if (!rt) {
        char *empty = malloc(sizeof(char) * 3);
        strcpy(empty, "{}");
        return empty;
    }

    char *name = (rt->name && rt->name[0] != '\0') ? rt->name : "None";
    char *isLoop = isLoopRoute(rt, 25) ? "true" : "false";
    char *otherData = gpxDataListToJSON(rt->otherData);
    float len = round10(getRouteLen(rt));
    int numPoints = rt->waypoints ? rt->waypoints->length : 0;

    char *str = malloc(sizeof(char) * (strlen(name)+sizeOfInt(numPoints)+sizeOfFloat(len)+strlen(isLoop) + 100));

    sprintf(str, "{\"name\":\"%s\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s,\"otherData\":%s}", 
            name, numPoints, len, isLoop, otherData);

    return str;

}

char *wpToJSON(const Waypoint *wp) {
    if (!wp) {
        char *empty = malloc(sizeof(char) * 3);
        strcpy(empty, "{}");
        return empty;
    }

    char *name = (wp->name && wp->name[0] != '\0') ? wp->name : "None";


    char *str =
        malloc(sizeof(char) * (strlen(name) + sizeOfFloat(wp->latitude) + sizeOfFloat(wp->longitude) + 150));

    sprintf(str,
            "{\"name\":\"%s\",\"latitude\":%f,\"longitude\":%f}",
            name, wp->latitude, wp->longitude);

    return str;
}

char *wpListToJSON(const List *wpList) {
    if (!wpList || wpList->length == 0) {
        char *empty = malloc(sizeof(char) * 3);
        strcpy(empty, "[]");
        return empty;
    }

    char *str = malloc(sizeof(char) * 3);
    strcpy(str, "[");

    ListIterator wpIter = createIterator((List *)wpList);
    Waypoint *wpPtr = NULL;

    int i = 0;
    while ((wpPtr = (Waypoint *)nextElement(&wpIter)) != NULL) {
        char *curr = wpToJSON(wpPtr);
        int newLen = strlen(str) + 50 + strlen(curr);
        str = realloc(str, newLen);
        strcat(str, curr);
        if (i != wpList->length - 1) {
            char *tmp = ",";
            strcat(str, tmp);
        }
        i++;
        free(curr);
    }

    char *tmp = "]";
    strcat(str, tmp);

    return str;
}

char *GPXDataToJSON(const GPXData *data){
    if (!data) {
        char *empty = malloc(sizeof(char) * 3);
        strcpy(empty, "{}");
        return empty;
    }
    
    char *str = malloc(sizeof(char) * (strlen(data->name) + strlen(data->value) + 100));

    sprintf(str, "{\"%s\":\"%s\"}",data->name, data->value);

    return str;
}

char *gpxDataListToJSON(const List *otherDataList) {
    if (!otherDataList || otherDataList->length == 0) {
        char *empty = malloc(sizeof(char) * 3);
        strcpy(empty, "[]");
        return empty;
    }

    char *str = malloc(sizeof(char) * 3);
    strcpy(str, "[");

    ListIterator odIter = createIterator((List *)otherDataList);
    GPXData *odPtr = NULL;

    int i = 0;
    while ((odPtr = (GPXData *)nextElement(&odIter)) != NULL) {
        char *curr = GPXDataToJSON(odPtr);
        int newLen = strlen(str) + 50 + strlen(curr);
        str = realloc(str, newLen);
        strcat(str, curr);
        if (i != otherDataList->length - 1) {
            char *tmp = ",";
            strcat(str, tmp);
        }
        i++;
        free(curr);
    }

    char *tmp = "]";
    strcat(str, tmp);

    return str;
}

char *routeListToJSON(const List *routeList){

    if (!routeList || routeList->length == 0) {
        char *empty = malloc(sizeof(char) * 3);
        strcpy(empty, "[]");
        return empty;
    }

    char *str = malloc(sizeof(char)*3);
    strcpy(str, "[");

    ListIterator routeIter = createIterator((List*)routeList);
    Route *routePtr = NULL;

    int i = 0;
    while ( (routePtr = (Route *)nextElement(&routeIter)) != NULL ){ 
        char *curr = routeToJSON(routePtr);
        int newLen = strlen(str)+50+strlen(curr);
        str = realloc(str, newLen);
        strcat(str, curr);
        if (i != routeList->length-1){
            char *tmp = ",";
            strcat(str, tmp);
        }
        i++;
        free(curr);
    }

    char *tmp = "]";
    strcat(str, tmp);

    return str;
}

char *trackListToJSON(const List *trackList) {
    if (!trackList || trackList->length == 0) {
        char *empty = malloc(sizeof(char) * 3);
        strcpy(empty, "[]");
        return empty;
    }

    char *str = malloc(sizeof(char)*3);
    strcpy(str, "[");

    ListIterator trackIter = createIterator((List *)trackList);
    Track *trackPtr = NULL;

    int i = 0;
    while ((trackPtr = (Track *)nextElement(&trackIter)) != NULL) {
        char *curr = trackToJSON(trackPtr);
        int newLen = strlen(str) + 50 + strlen(curr);
        str = realloc(str, newLen);
        strcat(str, curr);
        if (i != trackList->length - 1) {
            char *tmp = ",";
            strcat(str, tmp);
        }
        i++;
        free(curr);
    }

    char *tmp = "]";
    strcat(str, tmp);

    return str;
}

char *GPXtoJSON(const GPXdoc *gpx) {

    if (!gpx) {
        char *empty = malloc(sizeof(char) * 3);
        strcpy(empty, "{}");
        return empty;
    }

    char *creator =
        (!gpx->creator || gpx->creator[0] == '\0') ? "None" : gpx->creator;
    int numWp = getNumWaypoints(gpx);
    int numRoutes = getNumRoutes(gpx);
    int numTracks = getNumTracks(gpx);

    char *ret = malloc(sizeof(char)*(strlen(creator)+sizeOfFloat(gpx->version)+sizeOfInt(numWp)+sizeOfInt(numRoutes)+sizeOfInt(numTracks)+1000));
    sprintf(ret, "{\"version\":%.1f,\"creator\":\"%s\",\"numWaypoints\":%d,\"numRoutes\":%d,\"numTracks\":%d}",
            gpx->version, creator, numWp, numRoutes, numTracks);

    return ret;
}

List *getRouteList(const GPXdoc *doc) { return doc->routes; }
List *getTrackList(const GPXdoc *doc) { return doc->tracks; }

int updateTrackName(GPXdoc *doc, char *fileName, char *oldName, char *newName) {
    if(!doc) return 0;
    Track *tr = getTrack(doc, oldName);
    tr->name = realloc(tr->name, strlen(newName) + 50);
    strcpy(tr->name, newName);
    writeGPXdoc(doc, fileName);
    return 1;
}

int updateRouteName(GPXdoc *doc, char *fileName, char *oldName, char *newName) {
    if (!doc) return 0;
    Route *rt = getRoute(doc, oldName);
    rt->name = realloc(rt->name, strlen(newName) + 50);
    strcpy(rt->name, newName);
    writeGPXdoc(doc, fileName);
    return 1;
}

void addRoute(GPXdoc *doc, Route *rt){
    if(!doc || !rt) return;
    insertBack(doc->routes, rt);
}

int addRouteWrapper(GPXdoc *doc, char *fileName, char *routeName, float latitudes[], float longitudes[], int numPoints) {

    Route *newRoute = malloc(sizeof(Route));
    newRoute->waypoints =
        initializeList(waypointToString, deleteWaypoint, compareWaypoints);
    newRoute->otherData =
        initializeList(gpxDataToString, deleteGpxData, compareGpxData);
    newRoute->name = malloc(sizeof(char)*strlen(routeName)+10);

    strcpy(newRoute->name, routeName);

    int i;
    for(i = 0; i < numPoints; i++){
        Waypoint *tmp = malloc(sizeof(Waypoint));
        tmp->otherData =
            initializeList(gpxDataToString, deleteGpxData, compareGpxData);
        tmp->name = malloc(sizeof(char));
        strcpy(tmp->name, "");
        tmp->latitude = latitudes[i];
        tmp->longitude = longitudes[i];
        insertBack(newRoute->waypoints, tmp);
    }

    addRoute(doc, newRoute);
    if(!writeGPXdoc(doc, fileName)){
        printf("Here");
        return 0;
    } 

    return 1;
}

int writeNewGPXDoc(char *fileName, char *creator, char *namespace,
                   double version) {
    GPXdoc *doc = NULL;
    doc = malloc(sizeof(GPXdoc));

    strcpy(doc->namespace, namespace);
    doc->creator = malloc(strlen(creator)+1);
    strcpy(doc->creator, creator);
    doc->version = version;
    doc->waypoints = initializeList(waypointToString, deleteWaypoint, compareWaypoints);
    doc->routes = initializeList(routeToString, deleteRoute, compareRoutes);
    doc->tracks = initializeList(trackToString, deleteTrack, compareTracks);

    if(writeGPXdoc(doc, fileName)){
        free(doc);
        return 1;
    } else {
        free(doc);
        return 0;
    }
}

char *getRouteWpList(Route *rt) {
    return wpListToJSON(rt->waypoints);
}

// int main(int argc, char const *argv[])
// {
//     GPXdoc *doc = createValidGPXdoc(argv[1], "./validator/validator.xsd");
//     Route *rt = getFromFront(doc->routes);

//     char *s = getRouteWpList(rt);

//     printf("%s", getRouteWpList(rt));
//     free(s);
//     free(doc);
//     return 0;
// }

    // int main(int argc, char **argv) {

    //     Route *routePtrFirst = NULL, *routePtrLast = NULL;
    //     Track *trackPtrFirst = NULL, *trackPtrLast = NULL;

    //     printf("\n1. Testing 'createValidGPXdoc' with valid arguments...\n");
    //     GPXdoc *doc = createValidGPXdoc(argv[1], "./bin/testFiles/gpx.xsd");
    //     if(doc) printf("\tCreated valid GPXdoc.\n");
    //     else {
    //         printf("\tCreated invalid GPXdoc... exiting program.\n");
    //         return 0;
    //     }

    //     routePtrFirst = getFromFront(doc->routes);
    //     routePtrLast = getFromBack(doc->routes);

    //     trackPtrFirst = getFromFront(doc->tracks);
    //     trackPtrLast = getFromBack(doc->tracks);

    //     printf("\n2. Testing 'validateGPXDoc' with valid arguments...\n");
    //     bool isValid = validateGPXDoc(doc, "./bin/testFiles/gpx.xsd");
    //     if(isValid) printf("\tValid GPXdoc.\n");
    //     else printf("\tInvalid GPXdoc.\n");

    //     printf("\n3. Testing 'writeGPXdoc' with valid arguments...\n");
    //     bool didWrite = writeGPXdoc(doc, "output.gpx");
    //     if(didWrite) printf("\tSuccesfully wrote GPXdoc to fileName
    //     output.gpx.\n"); else printf("\tCouldn't write GPXdoc
    //     succesfully.\n");

    //     printf("\n4. Testing 'getRouteLen' with '%s'...\n",
    //     routePtrFirst->name); float routeLen = getRouteLen(routePtrFirst);
    //     printf("\t'%s' route len is %f, round10 is %f.\n",
    //     routePtrFirst->name, routeLen, round10(routeLen));

    //     printf("\n5. Testing getTrackLen with %s...\n", trackPtrFirst->name);
    //     float trackLen = getTrackLen(trackPtrFirst);
    //     printf("\t'%s' track len is %f. round10 is %f.\n",
    //     trackPtrFirst->name, trackLen, round10(trackLen));

    //     float delta = 11.0;
    //     printf("\n6. Testing numRoutesWithLength with '%f' and delta
    //     '%f'...\n",
    //            routeLen + delta, delta);
    //     int numRoutes = numRoutesWithLength(doc, routeLen+delta, delta);
    //     printf("\t%d routes with length %f +- %f.\n", numRoutes, routeLen,
    //     delta);

    //     delta = 10.0;
    //     printf("\n7. Testing numRoutesWithLength with '%f' and delta
    //     '%f'...\n", routeLen-delta-1, 10.0); numRoutes =
    //     numRoutesWithLength(doc, routeLen-delta-1, delta); printf("\t%d
    //     routes with length %f +- %f.\n", numRoutes, routeLen, delta+1);

    //     delta = 8.0;
    //     printf("\n8. Testing numTracksWithLength with '%f' and delta
    //     '%f'...\n", trackLen+delta, delta); int numTracks =
    //     numTracksWithLength(doc, trackLen+delta, delta); printf("\t%d routes
    //     with length %f +- %f.\n", numTracks, trackLen, delta);

    //     delta = 7.0;
    //     printf("\n9. Testing numTracksWithLength with '%f' and delta
    //     '%f'...\n",
    //            trackLen - delta - 1, delta);
    //     numTracks = numTracksWithLength(doc, trackLen-delta-1, delta);
    //     printf("\t%d routes with length %f +- %f.\n", numTracks, trackLen,
    //     delta);

    //     printf("\n10. Testing isLoopRoute with '%s' and delta 10...\n",
    //     routePtrFirst->name); isLoopRoute(routePtrFirst, 10.0)
    //         ? printf("\t'%s' is loop route.\n", routePtrFirst->name)
    //         : printf("\t '%s' isn't loop route.\n", routePtrFirst->name);

    //     printf("\n10.1 Testing isLoopRoute with '%s' and delta 10...\n",
    //            routePtrLast->name);
    //     isLoopRoute(routePtrLast, 10.0)
    //         ? printf("\t'%s' is loop route.\n", routePtrLast->name)
    //         : printf("\t '%s' isn't loop route.\n", routePtrLast->name);

    //     printf("\n11. Testing isLoopRoute with '%s' and delta 10000...\n",
    //     routePtrFirst->name); isLoopRoute(routePtrFirst, 10000.0)
    //         ? printf("\t'%s' is loop route.\n", routePtrFirst->name)
    //         : printf("\t '%s' isn't loop route.\n", routePtrFirst->name);

    //     printf("\n12. Testing isLoopTrack with '%s' and delta 10...\n",
    //            trackPtrFirst->name);
    //     isLoopTrack(trackPtrFirst, 10.0)
    //         ? printf("\t'%s' is loop track.\n", trackPtrFirst->name)
    //         : printf("\t '%s' isn't loop track.\n", trackPtrFirst->name);

    //     printf("\n12.1 Testing isLoopTrack with '%s' and delta 10...\n",
    //            trackPtrLast->name);
    //     isLoopTrack(trackPtrLast, 10.0)
    //         ? printf("\t'%s' is loop track.\n", trackPtrLast->name)
    //         : printf("\t '%s' isn't loop track.\n", trackPtrLast->name);

    //     printf("\n13. Testing isLoopTrack with '%s' and delta 10000...\n",
    //            trackPtrFirst->name);
    //     isLoopTrack(trackPtrFirst, 10000.0)
    //         ? printf("\t'%s' is loop track.\n", trackPtrFirst->name)
    //         : printf("\t '%s' isn't loop track.\n", trackPtrFirst->name);

    //     Waypoint *firstWp = getFromFront(routePtrFirst->waypoints);
    //     Waypoint *secondWp = getFromBack(routePtrLast->waypoints);

    //     printf(
    //         "\n14. Testing getRoutesBetween with sourceLat=%f, sourceLon=%f,
    //         " "destLat=%f, destLong=%f and delta=%f\n", firstWp->latitude,
    //         firstWp->longitude, secondWp->latitude,
    //         secondWp->longitude, 5.0);
    //     List *routesBetween =
    //         getRoutesBetween(doc, firstWp->latitude, firstWp->longitude,
    //                          secondWp->latitude, secondWp->longitude, 5.0);
    //     routesBetween
    //         ? printf("\tLength of routes between is %d.\n",
    //         routesBetween->length) : printf("\tList was NULL.\n");

    //     printf(
    //         "\n15. Testing getRoutesBetween with sourceLat=%f, sourceLon=%f,
    //         " "destLat=%f, destLong=%f and delta=%f\n", firstWp->latitude,
    //         firstWp->longitude, 44.596100, 123.620570, 5.0);
    //     List *routesBetween2 = getRoutesBetween(
    //         doc, firstWp->latitude, firstWp->longitude, 44.596100,
    //         -123.620570, 5.0);
    //     routesBetween2
    //         ? printf("\tLength of routes between is %d.\n",
    //         routesBetween2->length) : printf("\tList was NULL.\n");

    //     TrackSegment *firstTrackSeg = getFromFront(trackPtrFirst->segments);
    //     TrackSegment *lastTrackSeg = getFromBack(trackPtrLast->segments);

    //     firstWp = getFromFront(firstTrackSeg->waypoints);
    //     secondWp = getFromBack(lastTrackSeg->waypoints);

    //     printf(
    //         "\n16. Testing getTracksBetween with sourceLat=%f, sourceLon=%f,
    //         " "destLat=%f, destLong=%f and delta=%f\n", firstWp->latitude,
    //         firstWp->longitude, secondWp->latitude,
    //         secondWp->longitude, 5.0);
    //     List *tracksBetween =
    //         getTracksBetween(doc, firstWp->latitude, firstWp->longitude,
    //                          secondWp->latitude, secondWp->longitude, 5.0);
    //     tracksBetween
    //         ? printf("\tLength of tracks between is %d.\n",
    //         tracksBetween->length) : printf("\tList was NULL.\n");

    //     printf(
    //         "\n17. Testing getTracksBetween with sourceLat=%f, sourceLon=%f,
    //         " "destLat=%f, destLong=%f and delta=%f\n", firstWp->latitude,
    //         firstWp->longitude, 49.598070, -123.620460, 5.0);
    //     List *tracksBetween2 = getTracksBetween(doc, firstWp->latitude,
    //     firstWp->longitude,
    //                                      49.598070, -123.620460, 5.0);
    //     tracksBetween2
    //         ? printf("\tLength of tracks between is %d.\n",
    //         tracksBetween2->length) : printf("\tList was NULL.\n");

    //     char *t1ToJson = trackToJSON(trackPtrFirst);
    //     char *t2ToJson = trackToJSON(trackPtrLast);
    //     char *r1ToJson = routeToJSON(routePtrFirst);
    //     char *r2ToJson = routeToJSON(routePtrLast);
    //     char *rlToJson = routeListToJSON(doc->routes);
    //     char *tlToJson = trackListToJSON(doc->tracks);
    //     char *docToJson = GPXtoJSON(doc);

    //     printf("\n18. Testing trackToJson with '%s'...\n",
    //     trackPtrFirst->name); printf("\t'%s' is output string.\n", t1ToJson);
    //     printf("\n19. Testing trackToJson with '%s'...\n",
    //     trackPtrLast->name); printf("\t'%s' is output string.\n", t2ToJson);
    //     printf("\n20. Testing trackToJson with '%s'...\n",
    //     routePtrFirst->name); printf("\t'%s' is output string.\n", r1ToJson);
    //     printf("\n21. Testing trackToJson with '%s'...\n",
    //     routePtrLast->name); printf("\t'%s' is output string.\n", r2ToJson);
    //     printf("\n22. Testing routeListToJson...\n");
    //     printf("\t'%s' is output string.\n", rlToJson);
    //     printf("\n23. Testing trackListToJson...\n");
    //     printf("\t'%s' is output string.\n", tlToJson);
    //     printf("\n24. Testing docToJson...\n");
    //     printf("\t'%s' is output string.\n", docToJson);

    //     free(t1ToJson);
    //     free(t2ToJson);
    //     free(r1ToJson);
    //     free(r2ToJson);
    //     free(rlToJson);
    //     free(tlToJson);
    //     free(docToJson);

    //     freeList(routesBetween);
    //     freeList(tracksBetween);
    //     freeList(routesBetween2);
    //     freeList(tracksBetween2);

    //     deleteGPXdoc(doc);

    //     printf("\n\n");

    //     return 0;
    // }

    // export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
