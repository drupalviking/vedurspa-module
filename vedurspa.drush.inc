<?php
/**
 * Created by PhpStorm.
 * User: drupalviking
 * Date: 09/08/16
 * Time: 10:35
 */
function vedurspa_drush_command() {
  $items['vedurspa-fetch-and-create-stations'] = array(
    'description' => dt('Fetches all stations from Veðurstofa and creates them as content types'),
    'arguments' => array(),
    'aliases' => array('vcreate'),
  );

  $items['vedurspa-fetch-forecasts'] = array(
    'aliases' => array('vforecasts'),
  );

  $items['vedurspa-fetch-observations'] = array(
    'aliases' => array('vobs'),
  );

  $items['vedurspa-run-temp'] = array(
    'aliases' => array('vtemp'),
  );

  return $items;
}

function drush_vedurspa_fetch_and_create_stations(){
  $links = vedurspa_fetch_station_info_links();
  $stations = vedurspa_process_stations($links);
  vedurspa_add_stations($stations);
}

/**
 * If you need a platform to run certain commands, you can use this function
 * @return array|bool
 */
function drush_vedurspa_run_temp(){

}

/**
 * Fetches all forecasts for the weather stations and stores them in the content
 * type
 *
 * @throws \Exception
 */
function drush_vedurspa_fetch_forecasts(){
  $string = _vedurspa_get_stations_string();

  $xml_string = simplexml_load_file(variable_get('vedurspa_web_service') . '&type=forec&lang=is&view=xml&ids=' . $string);
  $obj = json_decode(json_encode($xml_string));

  foreach($obj->station as $data){
    $id = $data->{'@attributes'}->id;
    $valid = $data->{'@attributes'}->valid;
    $node = _vedurspa_find_station_by_id($id);
    if(!$valid){
      $node->field_gild_vedurspa[LANGUAGE_NONE][0]['value'] = 0;
      //Pæling...Athuga atime á veðurspánni og skrá það kannski með.  Ef
      //of langt er orðið frá veðurspánni (12-24 klst) má hætta að birta hana
      //eða birta hana með fyrirvara.
    }
    else{
      //We have forecast data
      $forecast_json = _vedurspa_jsonfy_forecast_data($data);
      $node->field_gild_vedurspa[LANGUAGE_NONE][0]['value'] = 1;
      $node->field_vedurspa_timi[LANGUAGE_NONE][0]['value'] = strtotime($data->atime);

      if(isset($forecast_json['harmonie'])){
        $node->field_vedurspa_harmonie[LANGUAGE_NONE][0]['value'] = $forecast_json['harmonie'];
        $node->field_vedurspa_harmonie[LANGUAGE_NONE][0]['safe_value'] = htmlentities($forecast_json['harmonie']);
      }

      $node->field_vedurspa_ecm_three[LANGUAGE_NONE][0]['value'] = $forecast_json['ecm_three'];
      $node->field_vedurspa_ecm_three[LANGUAGE_NONE][0]['safe_value'] = htmlentities($forecast_json['ecm_three']);

      $node->field_vedurspa_ecm_six[LANGUAGE_NONE][0]['value'] = $forecast_json['ecm_six'];
      $node->field_vedurspa_ecm_six[LANGUAGE_NONE][0]['safe_value'] = htmlentities($forecast_json['ecm_six']);
    }

    node_save($node);
  }
}

/**
 * Fetches all observations for the weather stations and stores them in the
 * content type
 *
 * @throws \Exception
 */
function drush_vedurspa_fetch_observations(){
  $string = _vedurspa_get_stations_string();
  $query_string = variable_get('vedurspa_web_service') . '&type=obs&lang=is&view=xml&ids=' . $string;

  $xml_string = simplexml_load_file($query_string);
  $obj = json_decode(json_encode($xml_string));

  foreach($obj->station as $data){
    $id = $data->{'@attributes'}->id;
    $valid = $data->{'@attributes'}->valid;
    $node = _vedurspa_find_station_by_id($id);

    if(!$valid) {
      $node->field_gild_vedurathugun[LANGUAGE_NONE][0]['value'] = 0;
    }
    else{
      $node->field_gild_vedurathugun[LANGUAGE_NONE][0]['value'] = 1;
      if(is_numeric($data->F)){
        $node->field_athugun_styrkur[LANGUAGE_NONE][0]['value'] = $data->F;
      }
      if(is_string($data->D)){
        $node->field_athugun_stefna[LANGUAGE_NONE][0]['value'] = $data->D;
      }
      if(is_numeric($data->FX)){
        $node->field_athugun_max_styrkur[LANGUAGE_NONE][0]['value'] = $data->FX;
      }
      if(is_numeric($data->FG)){
        $node->field_athugun_max_hvida[LANGUAGE_NONE][0]['value'] = $data->FG;
      }
      if(is_string(str_replace(',', '.', $data->T))){
        $node->field_athugun_hitastig[LANGUAGE_NONE][0]['value'] = $data->T;
      }
      if(is_string($data->W)){
        $node->field_athugun_vedurlysing[LANGUAGE_NONE][0]['value'] = $data->W;
      }

      // These parameters do not always come with the XML data.  Therefor
      // there is a chance that we have stale data within the field.
      // That's why we have else statement that set's the field value to null
      if(is_numeric($data->V)){
        $node->field_athugun_skyggni[LANGUAGE_NONE][0]['value'] = (string) $data->V;
      }
      else{
        $node->field_athugun_skyggni = null;
      }

      if(is_string($data->N)){
        $node->field_athugun_skyjahula[LANGUAGE_NONE][0]['value'] = $data->N;
      }
      else{
        $node->field_athugun_skyjahula = null;
      }

      if(is_string($data->P)){
        $node->field_athugun_loftthrystingur[LANGUAGE_NONE][0]['value'] = $data->P;
      }
      else{
        $node->field_athugun_loftthrystingur = null;
      }

      if(is_string($data->RH)){
        $node->field_athugun_rakastig[LANGUAGE_NONE][0]['value'] = $data->RH;
      }
      else{
        $node->field_athugun_rakastig = null;
      }

      if(is_string($data->SNC)){
        $node->field_athugun_lysing_snjo[LANGUAGE_NONE][0]['value'] = $data->SNC;
      }
      else{
        $node->field_athugun_lysing_snjo = null;
      }

      if(is_string($data->SND)){
        $node->field_athugun_snjodypt[LANGUAGE_NONE][0]['value'] = $data->SND;
      }
      else{
        $node->field_athugun_snjodypt = null;
      }

      if(is_string($data->SED)){
        $node->field_athugun_snjolag[LANGUAGE_NONE][0]['value'] = $data->SED;
      }
      else{
        $node->field_athugun_snjolag = null;
      }

      if(is_string($data->RTE)){
        $node->field_athugun_vegahiti[LANGUAGE_NONE][0]['value'] = $data->RTE;
      }
      else{
        $node->field_athugun_vegahiti = null;
      }

      if(is_string($data->TD)){
        $node->field_athugun_daggarmark[LANGUAGE_NONE][0]['value'] = $data->TD;
      }else{
        $node->field_athugun_daggarmark = null;
      }

      if(is_string($data->R)){
        $node->field_athugun_upps_urkoma[LANGUAGE_NONE][0]['value'] = str_replace(',', '.', $data->R);
      }
      else{
        $node->field_athugun_upps_urkoma = null;
      }
    }

    $node->field_athugun_timi[LANGUAGE_NONE][0]['value'] = strtotime($data->time);
    node_save($node);
  }
}

/**
 * Fetches the webpage from Veðurstofa Íslands, that contains a list of all
 * weather stations, uses simple_html_dom to iterate through it and finds all
 * information pages buried within.  Adds the links to those pages to an array
 * and returns the array
 *
 * @return array of links to information pages for weather stations
 */
function vedurspa_fetch_station_info_links(){
  $page = file_get_html('http://www.vedur.is/vedur/stodvar?t=3');
  $links = [];
  foreach($page->find('td') as $elements){
    foreach($elements->nodes as $node){
      $found_title = strpos($node->attr['title'], '- Upplýsingar');
      if($found_title !== false){
        $links[] = $node->attr['href'];
      }
    }
  }

  return $links;
}

/**
 * Takes an array of links, fetches each page and extracts the data from the
 * pages.
 *
 * @param $links
 * @return array $stations
 */
function vedurspa_process_stations($links){
  $stations = [];
  foreach($links as $link){
    $station = new stdClass();
    $page = file_get_html("http://www.vedur.is" . $link);
    foreach($page->find('tr') as $elements){
      switch($elements->nodes[0]->nodes[0]->_[4]){
        case "Nafn" :
          $station->name = $elements->nodes[1]->nodes[0]->_[4];
          break;
        case "Tegund" :
          $station->type = new stdClass();
          $station->type->human_string = $elements->nodes[1]->nodes[0]->_[4];
          switch($station->type) {
            case strcmp($station->type->human_string, 'Sjálfvirk veðurathugunarstöð') == 0 :
              $station_type = 'automatic';
              break;
            case strcmp($station->type->human_string, 'Mönnuð skeytastöð') == 0 :
              $station_type = 'human';
              break;
            default:
              $station_type = 'other';
          }
          $station->type->machine_name = $station_type;
          break;
        case "Stöðvanúmer" :
          $station->station_number = $elements->nodes[1]->nodes[0]->_[4];
          break;
        case "WMO-númer" :
          $station->wmo_number = $elements->nodes[1]->nodes[0]->_[4];
          break;
        case "Skammstöfun" :
          $station->abbr = $elements->nodes[1]->nodes[0]->_[4];
          break;
        case "Spásvæði" :
          $station->forcast_zone = new stdClass();
          $forcast_zone_string = $elements->nodes[1]->nodes[0]->_[4];
          $opening_brace = strpos($forcast_zone_string, '(');
          $station->forcast_zone->name = substr($forcast_zone_string, 0, $opening_brace);
          $station->forcast_zone->abbr = substr($forcast_zone_string, $opening_brace+1, 2);
          break;
        case "Staðsetning" :
          $station->location = new stdClass();
          $location_string = $elements->nodes[1]->nodes[0]->_[4];

          $opening_brace = strpos($location_string, '(');
          $closing_brace = strpos($location_string, ')');

          $degree_string = substr($location_string, 0, $opening_brace);
          $degree_array = explode(',', $degree_string);
          $decimal_string = substr($location_string, $opening_brace+1, $closing_brace - ($opening_brace + 1));
          $decimal_array = explode(',', $decimal_string);

          $station->location->deg_lat = trim($degree_array[0]);
          $station->location->deg_lng = (trim($degree_array[1])) * -1;
          $station->location->dec_lat = trim($decimal_array[0] . "." . $decimal_array[1]);
          $station->location->dec_lng = (trim($decimal_array[2] . "." . $decimal_array[3])) * -1;
          break;
        case "Hæð yfir sjó" :
          $station->elevation = doubleval($elements->nodes[1]->nodes[0]->_[4]);
          break;
        case "Upphaf veðurathuguna" :
          $station->start_date_of_discovery = $elements->nodes[1]->nodes[0]->_[4];
          break;
        case "Eigandi stöðvar" :
          $station->owner = $elements->nodes[1]->nodes[0]->_[4];
          break;
      }
    }

    $stations[] = $station;
  }

  return $stations;
}

/**
 * Adds/replaces information about weather stations in the content type Veðurstöð.
 *
 * @param $stations
 */
function vedurspa_add_stations($stations){
  foreach($stations as $station){
    //Search for the node.  If search result is empty, we create one, if it finds
    //something, we add to it.
    $node = _vedurspa_search_station($station->station_number);
    if(!$node){
      $node = new stdClass();
      $node->title = $station->name;
      $node->type = 'vedurstod';
      node_object_prepare($node);
    }
    else{
      $node = node_load($node->nid);
    }

    $node->language = 'und';
    $node->uid = 1;
    $node->field_vedurstodvartegund[LANGUAGE_NONE][0]['value'] = $station->type->machine_name;

    $node->field_stodvarnumer[LANGUAGE_NONE][0]['value'] = $station->station_number;
    $node->field_stodvarnumer[LANGUAGE_NONE][0]['format'] = null;
    $node->field_stodvarnumer[LANGUAGE_NONE][0]['safe_value'] = $station->station_number;

    $node->field_wmo_numer[LANGUAGE_NONE][0]['value'] = $station->wmo_number;
    $node->field_wmo_numer[LANGUAGE_NONE][0]['format'] = null;
    $node->field_wmo_numer[LANGUAGE_NONE][0]['safe_value'] = $station->wmo_number;

    $node->field_vedurstodvarskammstofun[LANGUAGE_NONE][0]['value'] = $station->abbr;
    $node->field_vedurstodvarskammstofun[LANGUAGE_NONE][0]['format'] = null;
    $node->field_vedurstodvarskammstofun[LANGUAGE_NONE][0]['safe_value'] = $station->abbr;

    $node->field_spasvaedi[LANGUAGE_NONE][0]['value'] = $station->forcast_zone->abbr;

    $node->field_lengd_gradur[LANGUAGE_NONE][0]['value'] = $station->location->deg_lat;
    $node->field_lengd_gradur[LANGUAGE_NONE][0]['format'] = null;
    $node->field_lengd_gradur[LANGUAGE_NONE][0]['safe_value'] = $station->location->deg_lat;

    $node->field_breidd_gradur[LANGUAGE_NONE][0]['value'] = $station->location->deg_lng;
    $node->field_breidd_gradur[LANGUAGE_NONE][0]['format'] = null;
    $node->field_breidd_gradur[LANGUAGE_NONE][0]['safe_value'] = $station->location->deg_lng;

    $node->field_lengd_decimal[LANGUAGE_NONE][0]['value'] = $station->location->dec_lat;
    $node->field_lengd_decimal[LANGUAGE_NONE][0]['format'] = null;
    $node->field_lengd_decimal[LANGUAGE_NONE][0]['safe_value'] = $station->location->dec_lat;

    $node->field_breidd_decimal[LANGUAGE_NONE][0]['value'] = $station->location->dec_lng;
    $node->field_breidd_decimal[LANGUAGE_NONE][0]['format'] = null;
    $node->field_breidd_decimal[LANGUAGE_NONE][0]['safe_value'] = $station->location->dec_lng;

    $node->field_haed_yfir_sjo[LANGUAGE_NONE][0]['value'] = $station->elevation;
    $node->field_haed_yfir_sjo[LANGUAGE_NONE][0]['format'] = null;
    $node->field_haed_yfir_sjo[LANGUAGE_NONE][0]['safe_value'] = $station->elevation;

    $node->field_upphaf_athugana[LANGUAGE_NONE][0]['value'] = $station->start_date_of_discovery;
    $node->field_upphaf_athugana[LANGUAGE_NONE][0]['format'] = null;
    $node->field_upphaf_athugana[LANGUAGE_NONE][0]['safe_value'] = $station->start_date_of_discovery;

    $node->field_eigandi_stodvar[LANGUAGE_NONE][0]['value'] = $station->owner;
    $node->field_eigandi_stodvar[LANGUAGE_NONE][0]['format'] = null;
    $node->field_eigandi_stodvar[LANGUAGE_NONE][0]['safe_value'] = $station->owner;


    $node = node_submit($node);
    node_save( $node );
  }
}

/**
 * Gets all weather stations and creates a semicolon-separated string with their
 * id's.
 *
 * @return null|string
 */
function _vedurspa_get_stations_string() {
  $stations = _vedurspa_get_all_stations();
  $string = NULL;
  foreach ($stations as $station) {
    if ($string) {
      $string .= ";";
    }
    $string .= $station->field_stodvarnumer[LANGUAGE_NONE][0]['value'];
  }
  return $string;
}

function _vedurspa_jsonfy_forecast_data($data){
  $size = sizeof($data->forecast);
  $index = 0;
  $arr = [];

  foreach($data->forecast as &$item){
    $precipitation = rand(0, 50);
    $clouds = _vedurspa_convert_text_cloud_descriptions_to_symbols($item->W, $item->ftime);

    $obj = new stdClass();
    $obj->{'@attributes'} = new stdClass();
    //The javascript parser wants this T in the string, or it won't render, even
    //if it removes it again.  I made a decision to add it here, rather than
    //"fixing" the javascript  @drupalviking
    $item->ftime = substr_replace($item->ftime, 'T', 10, 1);
    $ftimestamp = strtotime($item->ftime);
    $obj->{'@attributes'}->from = $item->ftime;
    $ttime = _vedurspa_get_to_timestamp($data->forecast, $index, $size);
    $item->ttime = substr_replace($ttime, 'T', 10, 1);
    $obj->{'@attributes'}->to = $item->ttime;
    $ttimestamp = strtotime($ttime);
    if($ttimestamp - $ftimestamp == 3600){
      $model = "Harmonie";
    }
    else if($ttimestamp - $ftimestamp == 10800){
      $model = "ECM_Three";
    }
    else{
      $model = "ECM_Six";
    }

    $obj->symbol = new stdClass();
    $obj->symbol->{'@attributes'} = new stdClass();
    $obj->symbol->{'@attributes'}->number = $item->cloud_symbol = $clouds['symbol'];
    $obj->symbol->{'@attributes'}->numberEx = $item->cloud_symbol = $clouds['numberex'];
    $obj->symbol->{'@attributes'}->name = $item->W;
    $obj->symbol->{'@attributes'}->var = $clouds['var'];

    $obj->precipitation = new stdClass();
    $obj->precipitation->{'@attributes'} = new stdClass();
    $obj->precipitation->{'@attributes'}->value = 0;
    $obj->precipitation->{'@attributes'}->minvalue = 0;
    $obj->precipitation->{'@attributes'}->maxvalue = 0;

    $obj->windDirection = new stdClass();
    $obj->windDirection->{'@attributes'} = new stdClass();
    $obj->windDirection->{'@attributes'}->deg = _vedurspa_convert_text_degrees_into_degrees($item->D);
    $obj->windDirection->{'@attributes'}->code = $item->D;
    $obj->windDirection->{'@attributes'}->name = $item->D;

    $obj->windSpeed = new stdClass();
    $obj->windSpeed->{'@attributes'} = new stdClass();
    $obj->windSpeed->{'@attributes'}->mps = $item->F;
    $obj->windSpeed->{'@attributes'}->name = "Vindstrengur";

    $obj->temperature = new stdClass();
    $obj->temperature->{'@attributes'} = new stdClass();
    $obj->temperature->{'@attributes'}->unit = "Celcius";
    $obj->temperature->{'@attributes'}->value = $item->T;

    $obj->pressure = new stdClass();
    $obj->pressure->{'@attributes'} = new stdClass();
    $obj->pressure->{'@attributes'}->unit = "hPa";
    $obj->pressure->{'@attributes'}->value = 0;

    $arr[$model][] = $obj;

    $index++;
  }

  $harmonie_object = _vedurspa_create_weather_object($data, $arr['Harmonie']);
  $ecm_three_object = _vedurspa_create_weather_object($data, $arr['ECM_Three']);
  $ecm_six_object = _vedurspa_create_weather_object($data, $arr['ECM_Six']);

  $json['harmonie'] = json_encode($harmonie_object);
  $json['ecm_three'] = json_encode($ecm_three_object);
  $json['ecm_six'] = json_encode($ecm_six_object);

  return $json;
}

/**
 * @param $data
 * @param $arr
 * @return \stdClass
 */
function _vedurspa_create_weather_object($data, $arr) {
  $object = new stdClass();
  $object->forecast = new stdClass();
  $object->forecast->tabular = new stdClass();
  $object->forecast->tabular->time = new stdClass();
  $object->forecast->tabular->time = $arr;

  $object->location = new stdClass();
  $object->location->name = $data->name;

  $object->credit = new stdClass();
  $object->credit->link = new stdClass();
  $object->credit->link->{'@attributes'} = new stdClass();
  $object->credit->link->{'@attributes'}->text = "Veðurgögn frá Veðurstofu Íslands";
  $object->credit->link->{'@attributes'}->url = "http://www.vedur.is";
  return $object;
}

function _vedurspa_debug($field_name = 'field_name', $bundle_name = 'article'){
  $entity_type = 'node';

  $info_config = field_info_field($field_name);
  $info_instance = field_info_instance($entity_type, $field_name, $bundle_name);
  unset($info_config['id']);
  unset($info_instance['id'], $info_instance['field_id']);
  include_once DRUPAL_ROOT . '/includes/utility.inc';
  $output = "field_create_field(" . drupal_var_export($info_config) . ");\n";
  $output .= "field_create_instance(" . drupal_var_export($info_instance) . ");";
  drupal_set_message("<textarea rows=30 style=\"width: 100%;\">". $output .'</textarea>');
}

/**
 * Converts string representation of wind direction into degrees.
 *
 * @param $text Wind direction in text format
 * @return int  Degrees
 */
function _vedurspa_convert_text_degrees_into_degrees($text){
  switch( $text ){
    case "N":
      return 0;
    case "NNA":
      return 22;
    case "NA":
      return 45;
    case "ANA":
      return 68;
    case "A":
      return 90;
    case "ASA":
      return 112;
    case "SA":
      return 135;
    case "SSA":
      return 158;
    case "S":
      return 180;
    case "SSV":
      return 202;
    case "SV":
      return 225;
    case "VSV":
      return 248;
    case "V":
      return 270;
    case "VNV":
      return 292;
    case "NV":
      return 315;
    case "NNV":
      return 338;
  }
}

function _vedurspa_convert_text_cloud_descriptions_to_symbols($text, $time){
  $day = _vedurspa_is_day($time);
  switch( $text ){
    case "Heiðskírt":
      return array( "symbol"=>1, "numberex"=>1, "var"=>($day) ? "01d" : "01n" );
    case "Léttskýjað":
      return array( "symbol"=>2, "numberex"=>2, "var"=>($day) ? "02d" : "02n" );
    case "Skýjað":
      return array( "symbol"=>3, "numberex"=>3, "var"=>($day) ? "03d" : "03n" );
    case "Alskýjað":
      return array( "symbol"=>4, "numberex"=>4, "var"=>"04" );
    case "Lítils háttar rigning":
      //Vantar betri merki
      return array( "symbol"=>1, "numberex"=>1, "var"=>($day) ? "01d" : "01n" );
    case "Rigning":
      return array( "symbol"=>9, "numberex"=>9, "var"=>"09" );
    case "Lítils háttar slydda":
      return array( "symbol"=>8, "numberex"=>44, "var"=>($day) ? "44d" : "44n" );
    case "Slydda":
      return array( "symbol"=>12, "numberex"=>12, "var"=>"12" );
    case "Lítils háttar snjókoma":
      return array( "symbol"=>13, "numberex"=>49, "var"=>"49" );
    case "Snjókoma":
      return array( "symbol"=>13, "numberex"=>13, "var"=>"13" );
    case "Skúrir":
      //Vantar betri merki
      return array( "symbol"=>1, "numberex"=>1, "var"=>($day) ? "01d" : "01n" );
    case "Slydduél":
      //Vantar betri merki
      return array( "symbol"=>1, "numberex"=>1, "var"=>($day) ? "01d" : "01n" );
    case "Snjóél":
      return array( "symbol"=>8, "numberex"=>8, "var"=>($day) ? "08d" : "08n" );
    case "Skýstrókar":
      //Vantar betri merki
      return array( "symbol"=>1, "numberex"=>1, "var"=>($day) ? "01d" : "01n" );
    case "Moldrok":
      //Vantar betri merki
      return array( "symbol"=>1, "numberex"=>1, "var"=>($day) ? "01d" : "01n" );
  }
}

function _vedurspa_is_day($time){
  $hr = substr($time, 11, 2);
  return ( $hr > 7 && $hr < 20 ) ? true : false;
}

function _vedurspa_get_to_timestamp($obj, $index, $size){
  if( $index != $size ){
    $item = $obj[$index];
    if( $index != $size -1 ){
      $next_item = $obj[$index+1];
      $ftime_to = $next_item->ftime;
    } else {
      //We're at the last item, and there is six hours between forecasts
      $ftime_to = $item->ftime;
      $date = strtotime( $ftime_to );
      $date = $date + (60 * 60 * 6);
      $ftime_to = strftime('%Y-%m-%d %H:%M:%S', $date);
    }
  }

  return $ftime_to;
}