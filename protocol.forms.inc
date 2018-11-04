<?php
include_once('tbs_class.php');
include_once('tbs_plugin_opentbs.php');

define('ORG', 'org');		//название таксономии
define('FORG', 'org_2');	//название поля организация
define('FPOS', 'position');	//название поле должность
define('FUNIT', 'unit');	//название поля подразделение
define('COMMON_PASS', 75);	//процент сдачи
define('FHOURS', 'hours');	//название поле количество часов
define('DELIMETER', '/');	//разделитель в системе - для винды сменить на \

function protocol_page_form($form, $form_state) {
    $form = array();
    $form['org'] = array(
	'#type' => 'select',
	'#title' => t('Организация'),
	'#options' => selectOrg(),
    );

    $form['course'] = array(
	'#type' => 'select',
	'#title' => t('Курс'),
	'#options' => selectCourse(),
    );

    $form['table-submit'] = array(
	'#type' => 'submit',
	'#value' => t('Показать'),
	'#ajax' => array(
	    'callback' => 'protocol_form_ajax_load_results',
	    'wrapper' => 'form-protocol-load-results',
	    'method' => 'replace',
	    'effect' => 'fade',
	),
    );

    $form['markup'] = array(
	'#prefix' => '<div id = "form-protocol-load-results">',
	'#suffix' => '</div>',
	'#markup' => '',
    );
    
    $form['protocol-submit'] = array(
	'#type' => 'submit',
	'#value' => t('Сформировать протокол'),
	'#ajax' => array(
	    'callback' => 'protocol_form_ajax_create_document',
	    'wrapper' => 'form-protocol-file-create',
	    'method' => 'replace',
	    'effect' => 'fade',
	),
    );

    $form['result-markup'] = array(
	'#prefix' => '<div id = "form-protocol-file-create">',
	'#suffix' => '</div>',
	'#markup' => '',
    );

    return $form;
}

function protocol_form_ajax_create_document($form, $form_state) {
    $ini = parse_ini_file('protocols.ini');
    $courseName = variable_get('courses');
    $courseId = variable_get('courses_id');
    $org_name = variable_get('organisation');
    $users = variable_get('user_array');
    
    $sql = 'SELECT field_' . FHOURS . '_value as h FROM field_data_field_' . FHOURS . ' WHERE entity_type=\'node\' AND entity_id=' . $courseId;
    $res = db_query($sql);
    $hours = 1;
    foreach($res as $r)
    {
	$hours = $r->h;
    }
    
    $TBS = new clsTinyButStrong;
    $TBS->Plugin(TBS_INSTALL, OPENTBS_PLUGIN);

    $TBS->VarRef['number'] = $ini['number'];
    $TBS->VarRef['day'] = date("d");
    $TBS->VarRef['month'] = getMonth(date("n"));
    $TBS->VarRef['year'] = date("Y");
    $TBS->VarRef['course'] = $courseName;
    $TBS->VarRef['hours'] = $hours;

    $table = array();
    $n = 1;

    foreach($users as $user) {
	$table[] = array('number' => $n, 'name' => $user['name'], 'position' => $user['position'], 'unit' => $user['unit'],);
	$n = $n + 1;
    }
    
    $template = $ini['template_dir'] . DELIMETER . 'Ptemplate.docx';
    $TBS->LoadTemplate($template, OPENTBS_ALREADY_UTF8);
    $TBS->MergeBlock('a', $table);
    $output_filename = $ini['result_dir'] . DELIMETER . 'protocol_№_' . $ini['number'] . '.docx';
    $TBS->Show(OPENTBS_FILE, $output_filename);
    setNumber();
    
    $resultString = 'Файл протокола создан. Файл находится в ' . $output_filename;
    $form['result-markup']['#markup'] = $resultString;
    return $form['result-markup'];
}

function setNumber() {
    $buf = parse_ini_file('protocols.ini');
    $buf['number'] = $buf['number'] + 1;
    $fileName = 'sites' . DELIMETER . 'all' . DELIMETER . 'modules' . DELIMETER . 'custom' . DELIMETER . 'protocol_module' . DELIMETER .'protocols.ini';
    $handle = fopen($fileName, 'w');
    fwrite($handle, 'number = ' . $buf['number'] . PHP_EOL);
    fwrite($handle, 'template_dir = ' . $buf['template_dir'] . PHP_EOL);
    fwrite($handle, 'result_dir = ' . $buf['result_dir']);
    fclose($handle);
}

function getMonth($n) {
    $month = array(
	'нулябрь', 'января', 'февраля', 'марта', 'апреля', 'мая', 'июня', 'июля', 'августа', 'сентября', 'октября', 'ноября', 'декабря',
    );

    return $month[$n];
}

function protocol_form_ajax_load_results($form, $form_state) {
    $form['markup']['#markup'] = createResultTable($form_state['input']['org'], $form_state['input']['course']);
    variable_set('organisation', $form_state['complete form']['org']['#options'][$form_state['input']['org']]);
    variable_set('courses', $form_state['complete form']['course']['#options'][$form_state['input']['course']]);
    variable_set('courses_id', $form_state['input']['course']);
    return $form['markup'];
}

function createResultTable($org, $course) {
    $users_id = array();
    $users_id = selectUsers($org);
    $course_quizes_vid = array();
    $course_quizes_vid = selectQuizes($course);
    $pass_rates = array();
    $pass_rates = selectPassRates($course_quizes_vid);
    $users = array();
    //information about users
    foreach ($users_id as $uid) {
	$sql = 'SELECT users.name as name, field_data_field_' . FPOS . '.field_' . FPOS . '_value as position, field_data_field_' . FUNIT . '.field_' . FUNIT . '_value as unit ';
	$sql = $sql . 'FROM users, field_data_field_' . FPOS . ', field_data_field_' . FUNIT . ' ';
	$sql = $sql . 'WHERE users.uid = ' . $uid . ' AND ';
	$sql = $sql . 'field_data_field_' . FPOS . '.entity_type = \'user\' AND field_data_field_' . FPOS . '.entity_id = ' . $uid . ' AND ';
	$sql = $sql . 'field_data_field_' . FUNIT . '.entity_type = \'user\' AND field_data_field_' . FUNIT . '.entity_id = ' . $uid;
	$user_result = db_query($sql);
	//print_r($user_result);
	foreach ($user_result as $user) {
		$users[$uid] = array(
		'name' => $user->name,
		'position' => $user->position,
		'unit' => $user->unit,
		'score' => 0,
		'pass' => 0,
	    );
	}
	//information about quizes pass
	$commonPass = 0;
	foreach($course_quizes_vid as $vid) {
	    $sql = 'SELECT score FROM quiz_node_results WHERE uid = ' . $uid . ' AND vid = ' . $vid;
	    $scoreResults = db_query($sql);
	    $maxScore = -1;
	    foreach($scoreResults as $score) {
		if ($maxScore < $score->score) {
		    $maxScore = $score->score;
		}
	    }
	    if ($maxScore >= $pass_rates[$vid]) $commonPass = $commonPass + 1;
	}

	$cP = ($commonPass * 100) / count($course_quizes_vid);
	$users[$uid]['score'] = $cP;
	if ($cP >= COMMON_PASS) $users[$uid]['pass'] = 1;
    }
    
    variable_set('user_array', $users);
    return drawTable($users);
}

function drawTable($uArray) {
    $table = '';
    $table = $table . '<table><caption>Результаты прохождения курса</caption>';
    $table = $table . '<tr><th>ФИО</th><th>Должность</th><th>Подразделение</th><th>Процент прохождения</th></tr>';
    foreach ($uArray as $user) {
	$row = '<tr>';
	if ($user['pass'] == 0) $row = '<tr bgcolor="red">';
	$row = $row . '<td>' . $user['name'] . '</td>';
	$row = $row . '<td>' .$user['position'] . '</td>';
	$row = $row . '<td>' . $user['unit'] . '</td>';
	$row = $row . '<td>' . $user['score'] . '%</td>';
	$row = $row . '</tr>';
	$table = $table . $row;
    }
    $table = $table . '</table>';
    return $table;
}

function selectUsers($org) {
    $result = array();
    $sql = 'SELECT entity_id FROM field_data_field_' . FORG . ' WHERE entity_type = \'user\' AND field_' . FORG . '_tid = ' . $org;
    $orgResult = db_query($sql);
    foreach($orgResult as $r)
    {
	$result[] = $r->entity_id;
    }
    return $result;
}

function selectQuizes($course) {
    $result = array();
    $sql = 'SELECT nid FROM book WHERE bid = ' . $course;
    $nodes = db_query($sql);
    foreach($nodes as $node)
    {
	$sql = 'SELECT vid FROM node WHERE type = \'quiz\' AND nid = ' . $node->nid;
	$quizes = db_query($sql);
	foreach($quizes as $quiz)
	{
	    $result[] = $quiz->vid;
	}
    }
    return $result;
}

function selectPassRates($vids) {
    $result = array();
    foreach($vids as $vid) {
	$sql = 'SELECT pass_rate FROM quiz_node_properties WHERE vid = ' . $vid;
	$passResult = db_query($sql);
	foreach($passResult as $pass) {
	    $result[$vid] = $pass->pass_rate;
	}
    }
    return $result;
}

function selectOrg() { 
    $org = array();
    $sql = 'SELECT vid FROM taxonomy_vocabulary WHERE machine_name = \''. ORG . '\'';
    $result = db_query($sql);
    foreach ($result as $r)
    {
	$sql = 'SELECT name, tid FROM taxonomy_term_data WHERE vid = ' . $r->vid;
	$result2 = db_query($sql);
	foreach ($result2 as $r2)
	{
	    $org[$r2->tid] = $r2->name;
	}
    }
    return $org;
}

function selectCourse() {
    $course = array();
    $sql = 'SELECT bid from book group by bid';
    $result = db_query($sql);
    foreach ($result as $r)
    {
	$sql = 'SELECT nid, title FROM node WHERE nid = ' . $r->bid;
	$result2 = db_query($sql);
	foreach($result2 as $r2)
	{
	    $course[$r2->nid] = $r2->title;
	}
    }

    return $course;
}
