<?php

$servername = "localhost";
$dbname = "id12051145_esp32testdatabase";
$username = "id12051145_esp32user";
$password = "Mamamia301";

$conn = new mysqli($servername, $username, $password, $dbname);

if ($conn->connect_error) {
  die("Connection failed: " . $conn->connect_error);
}

$sql = "SELECT vrijeme, datum FROM Seminarski_alarm WHERE id=(SELECT max(id) FROM Seminarski_alarm)";

if ($result = $conn->query($sql)) {
    while ($row = $result->fetch_assoc()) {
        $row_vrijeme = $row["vrijeme"];
		$row_datum = $row["datum"];
		
		//echo $row_vrijeme;
		//echo "\n";
		//echo $row_datum;
		//echo "\n";
		
		$EPOCH_datum = strtotime($row_datum);
		
		$EPOCH_vrijeme = explode(":", $row_vrijeme);
		//echo $EPOCH_vrijeme[0];
		//echo $EPOCH_vrijeme[1];
		//echo $EPOCH_vrijeme[2];
		
		
		$EPOCH_total = $EPOCH_datum + $EPOCH_vrijeme[0]*3600 + $EPOCH_vrijeme[1]*60 + $EPOCH_vrijeme[0];
		echo $EPOCH_total;
    }
    $result->free();
}
$conn->close();
?> 