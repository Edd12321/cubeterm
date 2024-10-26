<?php
function refresh()
{
	echo "<meta http-equiv='refresh' content='0' />";
}

function complain($message)
{
	echo 
		"<tr><td colspan=3>
		<hr />
		<font color=red>
			<b>$message</b>
		</font>
		</td></tr>";
	exit();
}
?>
