

#include "ship.h"

#include <string.h>
/* libxml2 needed later then 2.6 */
#include "libxml/xmlreader.h"

#include "log.h"


#define XML_NODE_START	1
#define XML_NODE_TEXT	3
#define XML_NODE_CLOSE	15
#define XML_NODE_CDATA	4

#define XML_ID		"Ships"	/* XML section identifier */
#define XML_SHIP	"ship"

static Ship* ship_stack = NULL;
static int ships;



/*
 * Gets a ship based on its name
 */
Ship* get_ship( const char* name )
{
	Ship* temp = ship_stack;

	while (temp != NULL)
		if (strcmp((temp++)->name, name)==0) break;

	return temp;
}


Ship* ship_parse( xmlNodePtr node )
{
	xmlNodePtr cur;
	Ship* temp = CALLOC_ONE(Ship);

	temp->name = (char*)xmlGetProp(node,(xmlChar*)"name");

	node = node->xmlChildrenNode;

	while ((node = node->next)) {
		if (strcmp((char*)node->name, "GFX")==0) {
			cur = node->children;
			if (strcmp((char*)cur->name,"text")==0)
				temp->gfx_ship = gl_newSprite((char*)cur->content, 6, 6);
		}
		else if (strcmp((char*)node->name, "class")==0) {
			cur = node->children;
			if (strcmp((char*)cur->name,"text")==0)
				temp->class = atoi((char*)cur->content);
		}
		else if (strcmp((char*)node->name, "movement")==0) {
			cur = node->children;
			while ((cur = cur->next)) {
				if (strcmp((char*)cur->name,"thrust")==0)
					temp->thrust = atoi((char*)cur->children->content);
				else if (strcmp((char*)cur->name,"turn")==0)
					temp->turn = atoi((char*)cur->children->content);
				else if (strcmp((char*)cur->name,"speed")==0)
					temp->speed = atoi((char*)cur->children->content);
			}
		}
		else if (strcmp((char*)node->name,"health")==0) {
			cur = node->children;
			while ((cur = cur->next)) {
				if (strcmp((char*)cur->name,"armor")==0)
					temp->armor = (FP)atoi((char*)cur->children->content);
				else if (strcmp((char*)cur->name,"shield")==0)
					temp->shield = (FP)atoi((char*)cur->children->content);
				else if (strcmp((char*)cur->name,"energy")==0)
					temp->energy = (FP)atoi((char*)cur->children->content);
				else if (strcmp((char*)cur->name,"armor_regen")==0)
					temp->armor_regen = (FP)(atoi((char*)cur->children->content))/60.0;
				else if (strcmp((char*)cur->name,"shield_regen")==0)
					temp->shield_regen = (FP)(atoi((char*)cur->children->content))/60.0;
				else if (strcmp((char*)cur->name,"energy_regen")==0)
					temp->energy_regen = (FP)(atoi((char*)cur->children->content))/60.0;
			}
		}
		else if (strcmp((char*)node->name,"caracteristics")==0) {
			cur = node->children;
			while((cur = cur->next)) {
				if (strcmp((char*)cur->name,"crew")==0)
					temp->crew = atoi((char*)cur->children->content);
				else if (strcmp((char*)cur->name,"mass")==0)
					temp->mass = (FP)atoi((char*)cur->children->content);
				else if (strcmp((char*)cur->name,"cap_weapon")==0)
					temp->cap_weapon = atoi((char*)cur->children->content);
				else if (strcmp((char*)cur->name,"cap_cargo")==0)
					temp->cap_cargo = atoi((char*)cur->children->content);
			}
		}
	}

	DEBUG("Loaded ship '%s'", temp->name);
	return temp;
}


int ships_load(void)
{
	xmlTextReaderPtr reader;
	xmlNodePtr node;
	Ship* temp = NULL;

	if ((reader=xmlNewTextReaderFilename(DATA))==NULL) {
		WARN("XML error reading "DATA);
		return -1;
	}

	/* get to the start of the "Ships" section */
	while (xmlTextReaderRead(reader)==1) {
		if (xmlTextReaderNodeType(reader)==XML_NODE_START &&
				strcmp((char*)xmlTextReaderConstName(reader),XML_ID)==0) break;
	}
	xmlTextReaderRead(reader); /* at Ships node */

	while (xmlTextReaderRead(reader)==1) {
		if (xmlTextReaderNodeType(reader)==XML_NODE_START &&
				strcmp((char*)xmlTextReaderConstName(reader),XML_SHIP)==0) {

			node = xmlTextReaderCurrentNode(reader); /* node to process */
			if (ship_stack==NULL) {
				ship_stack = temp = ship_parse(node);
				ships = 1;
			}
			else {
				temp = ship_parse(node);
				ship_stack = realloc(ship_stack, sizeof(Ship)*(++ships));
				memcpy(ship_stack+ships-1, temp, sizeof(Ship));
				free(temp);
			}
		}
	}

	xmlFreeTextReader(reader);

	return 0;
}

void ships_free()
{
	int i;
	for (i = 0; i < ships; i++) {
		if ((ship_stack+i)->name) 
			free((ship_stack+i)->name);
		gl_free((ship_stack+i)->gfx_ship);
	}
	free(ship_stack);
	ship_stack = NULL;
}

