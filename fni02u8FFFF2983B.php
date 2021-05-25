<html>
<body>
<?php
$servername = "localhost";
$dbname = "id12051145_esp32testdatabase";
$username = "id12051145_esp32user";
$password = "Mamamia301";

$conn = new mysqli($servername, $username, $password, $dbname);

if ($conn->connect_error) {
  die("Connection failed: " . $conn->connect_error);
}

$sql="INSERT INTO Seminarski_alarm (vrijeme, datum)

VALUES ('$_POST[vrijeme]','$_POST[datum]')";

if ($conn->query($sql)) {
  echo "Novi alarm uspješno dodan";
} else {
  echo "Greška: " . $sql . "<br>" . $conn->error;
}

$conn->close();
?>
</body>
</html>