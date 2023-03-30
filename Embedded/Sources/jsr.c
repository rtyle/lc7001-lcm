// JSR v1.1.1 - A stream-oriented JSON reader - Legrand 2019

#include "jsr.h"

// Clear match counts at all depths.  This is called by jsr_env_reset.
// It can also be called mid-stream to reset match counts.
void jsr_clear_match_counts(JSR_ENV* env)
{
	memset(&env->match_count, 0, sizeof(env->match_count));
}


// jsr_env_reset should be called on a JSR_ENV struct before jsr_read()
// is called on another stream.  It will reset state members, leaving 
// pointers and options intact.
void jsr_env_reset(JSR_ENV* env)
{
	jsr_clear_match_counts(env);
	env->state = 0;
	env->error = 0;
	env->read_idx = 0;
	env->write_idx = 0;
	env->depth = 0;
	env->fifo4 = 0;
}


// jsr_env_init should be called on a newly created JSR_ENV struct to 
// initialize its members. 
void jsr_env_init(JSR_ENV* env)
{
	memset(env, 0, sizeof(JSR_ENV));
}


// Internal function used by jsr_read()
static uint8_t jsr_match_pairs(JSR_ENV* env)
{
	if( !env->key_str[0] && !env->value_str[0] )
	{
		return JSR_RET_OK;
	}

	uint8_t p = 0;
	char match_flags = 0;

	#if(JSR_OPTION_ENABLE_SAFETIES)
	if(!env->pairs)
	{
		env->error |= JSR_ERROR_NULL_PAIR_PTR;
		return JSR_RET_FATAL_ERROR;
	}

	if(!env->key_str)
	{
		env->error |= JSR_ERROR_NULL_KEY_PTR;
		return JSR_RET_FATAL_ERROR;
	}

	if(!env->value_str)
	{
		env->error |= JSR_ERROR_NULL_VALUE_PTR;
		return JSR_RET_FATAL_ERROR;
	}
	#endif
	
	for(p = 0; p<env->pair_count; p++)
	{
		match_flags = 0;
		
		// Does this pair have a key defined?
		if(env->pairs[p].option & JSR_PAIR_KEY)
		{
			if(!strcmp(env->pairs[p].key_str, env->key_str))
			{
				match_flags |= JSR_PAIR_KEY;
			}
		}
		
		// Does this pair have a value defined?
		if(env->pairs[p].option & JSR_PAIR_VALUE)
		{
			if(!strcmp(env->pairs[p].value_str, env->value_str))
			{
				match_flags |= JSR_PAIR_VALUE;
			}
		}
		
		// Increment the match count if:
		// 1. A pair defining both value and key, has matched on both
		// 2. A pair defining either value or key has matched on the field it provides
		
		if( (env->pairs[p].option & JSR_PAIR_BOTH) == match_flags)
		{
			env->match_count[env->depth]++;
			env->pairs[p].depth = env->depth;
			#if(JSR_OPTION_PRINTF_DEBUG)
			printf("\njsr_match_pairs(): Match on %s:%s at depth %d", env->key_str, env->value_str, env->depth);
			#endif
		}
				
		// If this pair defines both a key and value, no copying to the pair will take place.
		if( (env->pairs[p].option & JSR_PAIR_BOTH) == JSR_PAIR_BOTH )
		{
						
		}
		else
		{	// At this point, the pair does not define both a key and value.
	
			if(! (env->pairs[p].option & JSR_PAIR_BOTH))
			{
				// If it defines neither, set the empty pair flag.
				#if(JSR_OPTION_ENABLE_SAFETIES)
				env->error |= JSR_ERROR_EMPTY_PAIR;
				#endif
			}
				
			if( match_flags & JSR_PAIR_KEY )
			{
				// If we've matched on the key, copy the value over to the pair.

				#if(JSR_OPTION_ENABLE_SAFETIES)
				if(!env->pairs[p].value_max_length)
				{
					env->error |= JSR_ERROR_ZERO_LENGTH_DEST;
					#if(JSR_OPTION_PRINTF_DEBUG)
					printf("\njsr_match_pairs(): WARNING - value_max_length==0 on pair %d", p);
					#endif
				}
				#endif
				
				strncpy(env->pairs[p].value_str, env->value_str, env->pairs[p].value_max_length);
			}

			if( match_flags & JSR_PAIR_VALUE )
			{
				// If we've matched on the value, copy the key over to the pair.

				#if(JSR_OPTION_ENABLE_SAFETIES)
				if(!env->pairs[p].key_max_length)
				{
					env->error |= JSR_ERROR_ZERO_LENGTH_DEST;
					#if(JSR_OPTION_PRINTF_DEBUG)
					printf("\njsr_match_pairs(): WARNING - key_max_length==0 on pair %d", p);
					#endif
				}
				#endif
				
				strncpy(env->pairs[p].key_str, env->key_str, env->pairs[p].key_max_length);
			}
		}
	}

	env->key_str[0] = 0;
	env->value_str[0] = 0;

	return JSR_RET_OK;
}


// The entry point for buffer processing
uint8_t jsr_read(JSR_ENV* env, char* buffer, uint32_t size)
{
	char c;
	int x;
	
	for(; env->read_idx<size; env->read_idx++)
	{
		c = buffer[env->read_idx];
		
		if(env->option & JSR_OPTION_SKIP_HTTP_HEADER)
		{
			env->fifo4 <<= 8;
			env->fifo4 |= c;

			if(env->fifo4 == JSR_HTTP_HEADER_DELIMITER)
			{
				env->state |= JSR_STATE_HTTP_BODY;
				#if(JSR_OPTION_PRINTF_DEBUG)
				printf("\njsr_read(): Reached HTTP body");
				#endif
			}

			if(!(env->state & JSR_STATE_HTTP_BODY))
			{
				// Don't process any more input
				//  if the HTTP body hasn't begun.
				continue;
			}
		}


		if(env->state & JSR_STATE_IN_QUOTES)
		{	// Evaluate characters within the context of a quoted string
			if(c == '"')
			{	// Quoted string has just ended
				env->state &= ~JSR_STATE_IN_QUOTES;	// Clear bit
			}	
			else
			{	// Within a quoted string - all characters except for " are copied.
				if(env->state & JSR_STATE_COLON)
				{	// Colon bit set means we're dealing with the value string
					if(env->write_idx < (env->value_max_length-1) )
					{
						env->value_str[env->write_idx++] = c;
						env->value_str[env->write_idx] = 0;
					}
					else
					{	// Indicate the value string has been truncated
						#if(JSR_OPTION_ENABLE_SAFETIES)
						env->error |= JSR_ERROR_VALUE_OVERFLOW;
						#endif
						
						#if(JSR_OPTION_PRINTF_DEBUG)
						printf("\njsr_read(): Truncated value string");
						#endif
					}
				}
				else
				{	// Colon bit cleared means we're dealing with the key string
					if(env->write_idx < (env->key_max_length-1) )
					{
						env->key_str[env->write_idx++] = c;
						env->key_str[env->write_idx] = 0;
					}
					else
					{	// Indicate the key string has been truncated
						#if(JSR_OPTION_ENABLE_SAFETIES)
						env->error |= JSR_ERROR_KEY_OVERFLOW;
						#endif
						
						#if(JSR_OPTION_PRINTF_DEBUG)
						printf("\njsr_read(): Truncated key string");
						#endif
					}
				}
			}
		}
		else	// Characters outside of a quoted string
		{
			if(c == '[')
			{
				env->array_depth++;
			}
			if(c == ']')
			{
				env->array_depth--;
			}
			if(c == ',')
			{
				jsr_match_pairs(env);	// Process the string pair

				env->state &= ~JSR_STATE_COLON;
				env->write_idx = 0;
			}
			else
			if(c == '"')
			{
				env->state |= JSR_STATE_IN_QUOTES;
				env->write_idx = 0;
			}
			else
			if(c == ':')
			{
				env->state |= JSR_STATE_COLON;
				env->write_idx=0;
			}
			else
			if(c == '{')
			{
				// Beginning of a new object

				if(env->depth < JSR_MAX_DEPTH)
				{
					env->depth++;
				}

				env->object_count++;

				if(env->depth==1)
				{
					env->top_level_object_count++;
				}

				// If a pair has the JSR_PAIR_TRANSIENT bit set, data captured to that pair will be
				// cleared when a new object begins at the same depth.  This prevents data from previous
				// objects from bleeding over to subsequent objects if those objects don't also have
				// the same matching members.  Retaining captured data across objects is sometimes
				// desirable, such as when searching for unique values.  Without JSR_PAIR_TRANSIENT set,
				// captured data will persist in the pair unless overwritten by a new match.  This
				// eliminates the need to set a low match_threshold and process matches mid-stream.

				for(x=0; x<env->pair_count; x++)
				{
					if(env->pairs[x].depth==env->depth && (env->pairs[x].option & JSR_PAIR_TRANSIENT))
					{
						if( (env->pairs[x].option & JSR_PAIR_BOTH) == JSR_PAIR_KEY )
						{
							// Key is provided, value is captured - clear value
							env->pairs[x].value_str[0] = 0;
						}

						if( (env->pairs[x].option & JSR_PAIR_BOTH) == JSR_PAIR_VALUE )
						{
							// Value is provided, key is captured - clear key
							env->pairs[x].key_str[0] = 0;
						}
					}
				}

				env->match_count[env->depth] = 0;
				env->state &= ~JSR_STATE_COLON;
			}
			else
			if(c == '}')
			{
				jsr_match_pairs(env);	// Process the string pair

				if(env->depth>0)
				{
					env->depth--;
				}

				if(env->option & JSR_OPTION_MATCH_COUNT_RECURSE)
				{
					env->match_count[env->depth] += env->match_count[env->depth+1];
				}

				if(env->depth >= env->match_min_depth)
				{
					if(env->match_count[env->depth+1] >= env->match_threshold)
					{
						#if(JSR_OPTION_PRINTF_DEBUG)
						printf("\njsr_read(): Match threshold reached: %d >= %d", env->match_count[env->depth], env->match_threshold);
						#endif
						env->read_idx++;

						// This tells the calling routine to evaluate the JSR_PAIR elements, and that we 
						// haven't finished processing the entire chunk of stream data.
						return JSR_RET_END_OBJECT;
					}
				}
			}
			else
			// Capture unquoted value strings (booleans, ints)
			if(c!=' ' && c!= '\t' && c!= '\n' && c!='\r' && c!='[' && c!=']')
			{
				if(env->state & JSR_STATE_COLON)
				{
					env->value_str[env->write_idx++] = c;
					env->value_str[env->write_idx] = 0;
				}
			}
		}
	}

	env->read_idx = 0;
	return JSR_RET_OK;
}
